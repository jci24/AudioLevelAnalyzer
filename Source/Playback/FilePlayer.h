#pragma once

#include "MeteredAudioSource.h"
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>

namespace audio
{
// Used on the message thread. JUCE handles output callbacks and buffered reading.
class FilePlayer
{
public:
    FilePlayer();
    ~FilePlayer();

    juce::String loadFile(const juce::File& file);
    void unload();
    juce::String play();
    void pause();
    void stop();

    bool hasFile() const;
    bool isPlaying() const;
    bool hasFinished() const;
    double positionSeconds() const;
    double durationSeconds() const;
    LivePlaybackLevels consumeLiveLevels();

private:
    juce::AudioFormatManager formats;
    juce::TimeSliceThread readAheadThread { "Audio file playback" };
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transport;
    MeteredAudioSource meteredSource { transport };
    juce::AudioSourcePlayer sourcePlayer;
    juce::AudioDeviceManager devices;
    bool outputConnected = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilePlayer)
};
}
