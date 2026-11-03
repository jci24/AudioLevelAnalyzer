#include "MainComponent.h"
#include "Analysis/SoundPressureLevel.h"

#include <cmath>
#include <cstdlib>
#include <exception>
#include <limits>

namespace
{
juce::String levelText(double value, double calibration, bool useSpl)
{
    const double db = useSpl ? audio::rmsToDbSpl(value, calibration)
        : value > 0.0 ? 20.0 * std::log10(value) : -std::numeric_limits<double>::infinity();
    return (std::isinf(db) ? juce::String("-inf") : juce::String(db, 2))
        + (useSpl ? " dB SPL" : " dBFS");
}

bool readCalibration(const juce::String& text, double& value)
{
    const auto input = text.trim().toStdString();
    char* end = nullptr;
    value = std::strtod(input.c_str(), &end);
    return end != input.c_str() && *end == '\0' && std::isfinite(value) && value > 0.0;
}
}

MainComponent::MainComponent()
{
    addAndMakeVisible(view);
    view.onOpen = [this] { chooseFile(); };
    view.onPlay = [this]
    {
        playbackError = player.play();
        updatePlaybackControls();
        updateLiveLevels();
    };
    view.onPause = [this]
    {
        player.pause();
        updatePlaybackControls();
        updateLiveLevels();
    };
    view.onStop = [this]
    {
        player.stop();
        updatePlaybackControls();
        updateLiveLevels();
    };
    view.onSettingsChanged = [this] { updateResults(); };
    updateResults();
    updatePlaybackControls();
    startTimerHz(30);
    setSize(780, 620);
}

MainComponent::~MainComponent()
{
    stopTimer();
    player.stop();
    worker.request_stop();
    if (worker.joinable())
        worker.join();
}

void MainComponent::resized() { view.setBounds(getLocalBounds()); }

void MainComponent::chooseFile()
{
    if (chooser != nullptr)
        return;
    chooser = std::make_unique<juce::FileChooser>("Choose an audio file", juce::File {},
        juce::String::fromUTF8(audio::audioFileWildcard().c_str()));
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [safe = juce::Component::SafePointer<MainComponent>(this)](const juce::FileChooser& dialog)
        {
            if (safe == nullptr)
                return;
            const auto selected = dialog.getResult();
            if (selected != juce::File {})
                safe->analyseFile(selected);
            safe->chooser.reset();
        });
}

void MainComponent::analyseFile(const juce::File& file)
{
    player.unload();
    selectedFile = file;
    playbackError.clear();
    updatePlaybackControls();
    updateLiveLevels();
    worker.request_stop();
    if (worker.joinable())
        worker.join();

    hasResult = false;
    updateResults();
    view.showFile(file);
    view.setLoading(true);

    worker = std::jthread([safe = juce::Component::SafePointer<MainComponent>(this),
                           path = file.getFullPathName().toStdString()](std::stop_token stop)
    {
        audio::FileAnalysis fileAnalysis;
        try
        {
            fileAnalysis = audio::analyseAudioFile(path, stop);
        }
        catch (const std::exception&)
        {
            fileAnalysis.error = "Could not analyse this file. Try another audio file.";
        }
        if (stop.stop_requested())
            return;

        juce::MessageManager::callAsync([safe, completed = std::move(fileAnalysis)]() mutable
        {
            if (safe == nullptr)
                return;
            safe->view.setLoading(false);
            safe->analysis = std::move(completed);
            safe->hasResult = safe->analysis.error.empty() && !safe->analysis.cancelled;
            if (safe->hasResult)
            {
                safe->playbackError = safe->player.loadFile(safe->selectedFile);
                const auto& data = safe->analysis;
                safe->view.showFileDetails(juce::String(data.sampleRate / 1000.0, 1) + " kHz / "
                    + juce::String(static_cast<int>(data.channels.size())) + " ch / "
                    + juce::String(static_cast<double>(data.samplesPerChannel) / data.sampleRate, 2) + " s");
            }
            else
            {
                safe->view.showFileDetails(juce::String::fromUTF8(safe->analysis.error.c_str()), true);
            }
            safe->updateResults();
            safe->updatePlaybackControls();
        });
    });
}

void MainComponent::updateResults()
{
    const bool useSpl = view.reference() != LevelReference::digital;
    double factor = 0.0;
    const bool valid = !useSpl || readCalibration(view.calibrationText(), factor);
    view.showReferenceValidity(valid);
    updateLiveLevels();
    view.showWholeFileLevels(
        hasResult && valid ? levelText(analysis.combined.peak, factor, useSpl) : "--",
        hasResult && valid ? levelText(analysis.combined.rms, factor, useSpl) : "--");
}

void MainComponent::timerCallback()
{
    updatePlaybackControls();
    updateLiveLevels();
}

void MainComponent::updateLiveLevels()
{
    const double now = juce::Time::getMillisecondCounterHiRes() / 1000.0;
    const double elapsed = lastMeterUpdateSeconds > 0.0 ? now - lastMeterUpdateSeconds : 0.0;
    lastMeterUpdateSeconds = now;
    const bool useSpl = view.reference() != LevelReference::digital;
    double factor = 0.0;
    const bool valid = !useSpl || readCalibration(view.calibrationText(), factor);
    const auto levels = player.consumeLiveLevels();
    const double referenceOffset = useSpl && valid ? audio::rmsToDbSpl(1.0, factor) : 0.0;
    for (int channel = 0; channel < 2; ++channel)
    {
        const bool available = valid && player.isPlaying() && channel < levels.channelCount;
        const auto reading = available ? levels.channels[static_cast<std::size_t>(channel)]
                                       : audio::LiveChannelLevels {};
        view.updateMeter(channel, reading, available, elapsed, referenceOffset);
    }
}

void MainComponent::updatePlaybackControls()
{
    juce::String position = "-- / --";
    if (player.hasFile())
    {
        const double seconds = juce::jmin(player.positionSeconds(), player.durationSeconds());
        position = juce::String(seconds, 2) + " / " + juce::String(player.durationSeconds(), 2) + " s";
    }
    view.showPlayback(player.hasFile(), player.isPlaying(),
                       player.isPlaying() || player.positionSeconds() > 0.0, position, playbackError);
}
