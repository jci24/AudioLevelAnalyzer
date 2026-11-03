#include "FilePlayer.h"

namespace audio
{
FilePlayer::FilePlayer()
{
    formats.registerBasicFormats();
    sourcePlayer.setSource(&meteredSource);
}

FilePlayer::~FilePlayer()
{
    transport.stop();
    devices.removeAudioCallback(&sourcePlayer);
    sourcePlayer.setSource(nullptr);
    unload();
    devices.closeAudioDevice();
    readAheadThread.stopThread(-1);
}

juce::String FilePlayer::loadFile(const juce::File& file)
{
    unload();
    std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(file));
    if (reader == nullptr || reader->lengthInSamples <= 0 || reader->sampleRate <= 0.0)
        return "Could not open the file for playback.";

    if (!readAheadThread.isThreadRunning() && !readAheadThread.startThread())
        return "Could not start the audio-file reading thread.";

    const double fileSampleRate = reader->sampleRate;
    auto source = std::make_unique<juce::AudioFormatReaderSource>(reader.get(), true);
    reader.release(); // The reader source now owns the decoder.
    constexpr int readAheadSamples = 32768;
    transport.setSource(source.get(), readAheadSamples, &readAheadThread, fileSampleRate);
    readerSource = std::move(source);
    return {};
}

void FilePlayer::unload()
{
    transport.stop();
    meteredSource.requestReset();
    transport.setSource(nullptr);
    readerSource.reset();
}

juce::String FilePlayer::play()
{
    if (!hasFile())
        return "Load an audio file first.";

    // Open output only when the user presses Play. No microphone input is requested.
    if (!outputConnected)
    {
        const auto error = devices.initialise(0, 2, nullptr, true);
        if (error.isNotEmpty())
            return "Could not open audio output: " + error;
        devices.addAudioCallback(&sourcePlayer);
        outputConnected = true;
    }

    if (hasFinished())
        transport.setPosition(0.0);
    meteredSource.requestReset();
    transport.start();
    return {};
}

void FilePlayer::pause()
{
    transport.stop(); // JUCE stops output while keeping the current position.
    meteredSource.requestReset();
}

void FilePlayer::stop()
{
    transport.stop();
    transport.setPosition(0.0);
    meteredSource.requestReset();
}

bool FilePlayer::hasFile() const { return readerSource != nullptr; }
bool FilePlayer::isPlaying() const { return transport.isPlaying(); }
bool FilePlayer::hasFinished() const { return hasFile() && transport.hasStreamFinished(); }
double FilePlayer::positionSeconds() const { return transport.getCurrentPosition(); }
double FilePlayer::durationSeconds() const { return transport.getLengthInSeconds(); }
LivePlaybackLevels FilePlayer::consumeLiveLevels() { return meteredSource.consumeLevels(); }
}
