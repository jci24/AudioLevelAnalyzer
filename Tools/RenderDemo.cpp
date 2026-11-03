#include "Application/AnalyzerView.h"
#include "Analysis/AudioFileAnalysis.h"
#include "Playback/MeteredAudioSource.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <filesystem>
#include <cmath>
#include <iostream>
#include <memory>

// Offline portfolio renderer: uses the real view, decoder, transport, and meters.
// No output device is opened. This is deliberately not presented as a screen recording.
int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI initialiseJuce;
    if (argc != 3)
    {
        std::cerr << "Usage: RenderDemo /absolute/path/Demo.wav /absolute/output/folder\n";
        return 1;
    }
    const juce::File file(juce::String::fromUTF8(argv[1]));
    const std::filesystem::path output(argv[2]);
    std::filesystem::create_directories(output);
    const auto analysis = audio::analyseAudioFile(file.getFullPathName().toStdString());
    if (!analysis.error.empty())
    {
        std::cerr << analysis.error << '\n';
        return 1;
    }
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(file));
    if (!reader)
        return 1;
    const double sampleRate = reader->sampleRate;
    juce::AudioFormatReaderSource source(reader.get(), false);
    juce::AudioTransportSource transport;
    audio::MeteredAudioSource metered(transport);
    transport.setSource(&source, 0, nullptr, sampleRate);
    constexpr int blockSamples = 480;
    metered.prepareToPlay(blockSamples, 48000.0);
    juce::AudioBuffer<float> audioBuffer(2, blockSamples);
    const juce::AudioSourceChannelInfo block(&audioBuffer, 0, blockSamples);

    AnalyzerView view;
    view.setSize(780, 620);
    view.showFile(file);
    const double duration = static_cast<double>(analysis.samplesPerChannel) / analysis.sampleRate;
    view.showFileDetails(juce::String(analysis.sampleRate / 1000.0, 1) + " kHz / "
        + juce::String(static_cast<int>(analysis.channels.size())) + " ch / "
        + juce::String(duration, 2) + " s");
    const auto db = [](double value)
    {
        return juce::String(20.0 * std::log10(value), 2) + " dBFS";
    };
    view.showWholeFileLevels(db(analysis.combined.peak), db(analysis.combined.rms));
    juce::PNGImageFormat png;
    const auto saveFrame = [&](const juce::File& destination, float scale)
    {
        juce::FileOutputStream stream(destination);
        if (!stream.openedOk() || !stream.setPosition(0) || stream.truncate().failed())
            return false;
        return png.writeImageToStream(
            view.createComponentSnapshot(view.getLocalBounds(), true, scale), stream);
    };

    transport.start();
    for (int frame = 0; frame < 100; ++frame)
    {
        // Ten real 10 ms audio blocks per 10 Hz animation frame.
        for (int index = 0; index < 10; ++index)
            metered.getNextAudioBlock(block);
        const auto levels = metered.consumeLevels();
        for (int channel = 0; channel < 2; ++channel)
        {
            view.updateMeter(channel, levels.channels[static_cast<std::size_t>(channel)],
                             channel < levels.channelCount, 0.1, 0.0);
        }
        view.showPlayback(true, transport.isPlaying(), transport.getCurrentPosition() > 0.0,
            juce::String(juce::jmin(transport.getCurrentPosition(), duration), 2)
                + " / " + juce::String(duration, 2) + " s", {});
        const auto path = output / ("frame-" + std::to_string(1000 + frame) + ".png");
        if (!saveFrame(juce::File(juce::String(path.string())), 1.0f))
            return 1;
        if (frame == 34 && !saveFrame(juce::File(juce::String((output / "analyzer.png").string())), 2.0f))
            return 1;
    }
    // Finish offline rendering before transport teardown; there is no device callback.
    for (int index = 0; index < 4000 && transport.isPlaying(); ++index)
        metered.getNextAudioBlock(block);
    metered.releaseResources();
    transport.setSource(nullptr);
    std::cout << output << '\n';
}
