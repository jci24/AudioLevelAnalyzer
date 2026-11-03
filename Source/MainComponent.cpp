#include "MainComponent.h"
#include "Audio/SoundPressureLevel.h"

#include <cmath>
#include <cstdlib>
#include <exception>
#include <limits>

namespace
{
const juce::Colour background { 0xff151c26 };
const juce::Colour foreground { 0xffedf2f7 };
const juce::Colour muted { 0xffb4c1d0 };
const juce::Colour accent { 0xff85d9be };
const juce::Colour errorColour { 0xffff9e94 };

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
    for (auto* label : { &fileInfo, &status, &summaryHeading, &peakHeading, &rmsHeading,
                        &peakValue, &rmsValue, &referenceLabel, &calibrationLabel,
                        &calibrationError, &liveHeading, &liveChannelOne, &liveChannelTwo,
                        &playbackStatus })
    {
        addAndMakeVisible(*label);
        label->setColour(juce::Label::textColourId, foreground);
        label->setFont(juce::FontOptions(14.0f));
    }
    fileInfo.setText("No file selected", juce::dontSendNotification);
    fileInfo.setFont(juce::FontOptions(19.0f, juce::Font::bold));
    status.setColour(juce::Label::textColourId, muted);
    status.setFont(juce::FontOptions(12.0f));
    summaryHeading.setText("Whole file", juce::dontSendNotification);
    for (auto* label : { &summaryHeading, &liveHeading })
        label->setFont(juce::FontOptions(16.0f, juce::Font::bold));
    peakHeading.setText("Peak", juce::dontSendNotification);
    rmsHeading.setText("RMS", juce::dontSendNotification);
    for (auto* label : { &peakHeading, &rmsHeading })
        label->setColour(juce::Label::textColourId, muted);
    for (auto* label : { &peakValue, &rmsValue })
        label->setFont(juce::FontOptions(24.0f));
    peakHeading.setTooltip("Largest sample magnitude across the complete file and all channels.");
    rmsHeading.setTooltip("Whole-file RMS combines all channel sample energies equally, without mixing channels.");
    for (auto* label : { &liveChannelOne, &liveChannelTwo })
    {
        label->setJustificationType(juce::Justification::centred);
        label->setColour(juce::Label::textColourId, muted);
    }
    liveChannelOne.setText("Output 1", juce::dontSendNotification);
    liveChannelTwo.setText("Output 2", juce::dontSendNotification);
    for (auto& meter : liveMeters)
        addAndMakeVisible(meter);

    for (auto* button : { &loadButton, &playButton, &pauseButton, &stopButton })
        addAndMakeVisible(*button);
    loadButton.setColour(juce::TextButton::textColourOffId, background);
    playButton.setColour(juce::TextButton::buttonColourId, accent);
    playButton.setColour(juce::TextButton::textColourOffId, background);
    loadButton.onClick = [this] { chooseFile(); };
    playButton.onClick = [this]
    {
        playbackError = player.play();
        updatePlaybackControls();
        updateLiveLevels();
    };
    pauseButton.onClick = [this]
    {
        player.pause();
        updatePlaybackControls();
        updateLiveLevels();
    };
    stopButton.onClick = [this]
    {
        player.stop();
        updatePlaybackControls();
        updateLiveLevels();
    };
    playbackStatus.setJustificationType(juce::Justification::centredRight);
    playbackStatus.setColour(juce::Label::textColourId, muted);

    referenceLabel.setText("Reference", juce::dontSendNotification);
    addAndMakeVisible(levelReference);
    levelReference.addItem("Digital (dBFS)", 1);
    levelReference.addItem("Calibrated SPL", 2);
    levelReference.addItem("Simulated SPL", 3);
    levelReference.setSelectedId(1, juce::dontSendNotification);
    levelReference.setTooltip("Digital RMS uses reference 1.0: a full-scale sine is -3.01 dBFS RMS. SPL requires a recording calibration; simulated SPL uses an assumed factor.");
    levelReference.onChange = [this]
    {
        updateResults();
        resized();
    };
    addChildComponent(calibration);
    calibration.setText("1.0");
    calibration.setTitle("Pascals per sample unit");
    calibration.setTooltip("Pascals represented by one sample unit. Use the factor for this recording's microphone chain and gain.");
    calibration.setSelectAllWhenFocused(true);
    calibration.onTextChange = [this] { updateResults(); };
    calibrationLabel.setText("Pa / unit", juce::dontSendNotification);
    calibrationError.setColour(juce::Label::textColourId, errorColour);
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
    fileInfo.setText(file.getFileName(), juce::dontSendNotification);
    fileInfo.setTooltip(file.getFullPathName());
    status.setColour(juce::Label::textColourId, muted);
    status.setText("Analysing...", juce::dontSendNotification);
    loadButton.setEnabled(false);

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
            safe->loadButton.setEnabled(true);
            safe->analysis = std::move(completed);
            safe->hasResult = safe->analysis.error.empty() && !safe->analysis.cancelled;
            if (safe->hasResult)
            {
                safe->playbackError = safe->player.loadFile(safe->selectedFile);
                const auto& data = safe->analysis;
                safe->status.setColour(juce::Label::textColourId, muted);
                safe->status.setText(juce::String(data.sampleRate / 1000.0, 1) + " kHz / "
                    + juce::String(static_cast<int>(data.channels.size())) + " ch / "
                    + juce::String(static_cast<double>(data.samplesPerChannel) / data.sampleRate, 2) + " s",
                    juce::dontSendNotification);
            }
            else
            {
                safe->status.setColour(juce::Label::textColourId, errorColour);
                safe->status.setText(juce::String::fromUTF8(safe->analysis.error.c_str()),
                                      juce::dontSendNotification);
            }
            safe->updateResults();
            safe->updatePlaybackControls();
        });
    });
}

void MainComponent::updateResults()
{
    const bool useSpl = levelReference.getSelectedId() != 1;
    double factor = 0.0;
    const bool valid = !useSpl || readCalibration(calibration.getText(), factor);
    calibration.setVisible(useSpl);
    calibrationLabel.setVisible(useSpl);
    calibrationError.setVisible(!valid);
    calibrationError.setText(valid ? "" : "Use a positive value", juce::dontSendNotification);
    liveHeading.setText(useSpl ? "Live (dB SPL)" : "Live (dBFS)", juce::dontSendNotification);
    updateLiveLevels();

    peakValue.setText(hasResult && valid ? levelText(analysis.combined.peak, factor, useSpl) : "--",
                       juce::dontSendNotification);
    rmsValue.setText(hasResult && valid ? levelText(analysis.combined.rms, factor, useSpl) : "--",
                      juce::dontSendNotification);
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
    const bool useSpl = levelReference.getSelectedId() != 1;
    double factor = 0.0;
    const bool valid = !useSpl || readCalibration(calibration.getText(), factor);
    const auto levels = player.consumeLiveLevels();
    const double referenceOffset = useSpl && valid ? audio::rmsToDbSpl(1.0, factor) : 0.0;
    for (int channel = 0; channel < 2; ++channel)
    {
        const auto index = static_cast<std::size_t>(channel);
        if (valid && player.isPlaying() && channel < levels.channelCount)
        {
            const auto& reading = levels.channels[index];
            liveMeters[index].setLevels(reading.peak, reading.rmsReady ? reading.rms : 0.0f,
                                         elapsed, referenceOffset, reading.rmsReady);
        }
        else
        {
            liveMeters[index].clear(referenceOffset);
        }
    }
}

void MainComponent::updatePlaybackControls()
{
    const bool loaded = player.hasFile();
    const bool playing = player.isPlaying();
    playButton.setEnabled(loaded && !playing);
    pauseButton.setEnabled(loaded && playing);
    stopButton.setEnabled(loaded && (playing || player.positionSeconds() > 0.0));
    loadButton.setColour(juce::TextButton::buttonColourId, loaded ? juce::Colour(0xffa9bbcd) : accent);

    playbackStatus.setColour(juce::Label::textColourId, playbackError.isNotEmpty() ? errorColour : muted);
    playbackStatus.setTooltip(playbackError);
    if (playbackError.isNotEmpty())
        playbackStatus.setText("Output unavailable", juce::dontSendNotification);
    else if (!loaded)
        playbackStatus.setText("-- / --", juce::dontSendNotification);
    else
    {
        const double position = juce::jmin(player.positionSeconds(), player.durationSeconds());
        playbackStatus.setText(juce::String(position, 2) + " / "
            + juce::String(player.durationSeconds(), 2) + " s", juce::dontSendNotification);
    }
}

void MainComponent::paint(juce::Graphics& graphics)
{
    graphics.fillAll(background);
    graphics.setColour(juce::Colour(0xff303d4c));
    graphics.drawHorizontalLine(184, 24.0f, static_cast<float>(getWidth() - 24));
    graphics.drawVerticalLine(244, 205.0f, static_cast<float>(getHeight() - 24));
}

void MainComponent::resized()
{
    const int width = getWidth();
    fileInfo.setBounds(24, 20, width - 164, 30);
    loadButton.setBounds(width - 124, 20, 100, 30);
    status.setBounds(24, 55, width - 48, 22);
    playButton.setBounds(24, 90, 78, 30);
    pauseButton.setBounds(110, 90, 78, 30);
    stopButton.setBounds(196, 90, 78, 30);
    playbackStatus.setBounds(294, 90, width - 318, 30);
    referenceLabel.setBounds(24, 140, 70, 28);
    levelReference.setBounds(98, 140, 205, 28);
    calibrationLabel.setBounds(323, 140, 65, 28);
    calibration.setBounds(393, 140, 90, 28);
    calibrationError.setBounds(493, 140, width - 517, 28);

    summaryHeading.setBounds(24, 205, 200, 25);
    peakHeading.setBounds(24, 250, 200, 22);
    peakValue.setBounds(24, 278, 200, 36);
    rmsHeading.setBounds(24, 340, 200, 22);
    rmsValue.setBounds(24, 368, 200, 36);
    liveHeading.setBounds(274, 205, width - 298, 25);
    const int channelWidth = (width - 318) / 2;
    liveChannelOne.setBounds(274, 240, channelWidth, 22);
    liveChannelTwo.setBounds(294 + channelWidth, 240, channelWidth, 22);
    liveMeters[0].setBounds(274, 270, channelWidth, getHeight() - 294);
    liveMeters[1].setBounds(294 + channelWidth, 270, channelWidth, getHeight() - 294);
}
