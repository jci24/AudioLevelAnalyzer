#pragma once

#include "Audio/LiveLevelMeter.h"
#include <juce_audio_devices/juce_audio_devices.h>
#include <array>
#include <atomic>
#include <cstdint>

namespace audio
{
struct LivePlaybackLevels
{
    std::array<LiveChannelLevels, 2> channels;
    int channelCount = 0;
};

// Tap the transport's rendered samples, after resampling and before device output.
// Audio processing owns the accumulators; the UI only reads published atomics.
class MeteredAudioSource final : public juce::AudioSource
{
public:
    explicit MeteredAudioSource(juce::AudioTransportSource& playbackTransport);
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& info) override;

    void requestReset(); // Message thread: do not touch the audio thread's accumulators.
    // Consume the largest peaks since the previous UI acknowledgement.
    LivePlaybackLevels consumeLevels();

private:
    void clearAudioThreadState(unsigned int generation);

    struct PublishedChannel
    {
        std::atomic<float> peak { 0.0f };
        std::atomic<float> rms { 0.0f };
        std::atomic<bool> rmsReady { false };
    };

    juce::AudioTransportSource& transport;
    std::array<LiveLevelMeter, 2> meters;
    std::array<PublishedChannel, 2> published;
    std::array<float, 2> pendingPeaks {};
    std::atomic<std::uint64_t> publicationSequence { 0 };
    std::atomic<std::uint64_t> consumedSequence { 0 };
    std::uint64_t audioThreadSequence = 0;
    std::atomic<int> publishedChannelCount { 0 };
    std::atomic<unsigned int> requestedGeneration { 0 };
    std::atomic<unsigned int> publishedGeneration { 0 };
    unsigned int audioThreadGeneration = 0;
    bool hadAudio = false;

    // No hidden mutex-backed atomics on the audio thread on supported builds.
    static_assert(std::atomic<float>::is_always_lock_free);
    static_assert(std::atomic<bool>::is_always_lock_free);
    static_assert(std::atomic<int>::is_always_lock_free);
    static_assert(std::atomic<unsigned int>::is_always_lock_free);
    static_assert(std::atomic<std::uint64_t>::is_always_lock_free);
};
}
