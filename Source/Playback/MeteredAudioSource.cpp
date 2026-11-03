#include "MeteredAudioSource.h"

namespace audio
{
MeteredAudioSource::MeteredAudioSource(juce::AudioTransportSource& playbackTransport)
    : transport(playbackTransport)
{
}

void MeteredAudioSource::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    transport.prepareToPlay(samplesPerBlockExpected, sampleRate);
    for (auto& meter : meters)
        meter.prepare(sampleRate);
    clearAudioThreadState(requestedGeneration.load());
}

void MeteredAudioSource::releaseResources()
{
    transport.releaseResources();
    clearAudioThreadState(requestedGeneration.load());
}

void MeteredAudioSource::clearAudioThreadState(unsigned int generation)
{
    for (auto& meter : meters)
        meter.reset();
    pendingPeaks.fill(0.0f);
    for (auto& channel : published)
    {
        channel.peak.store(0.0f);
        channel.rms.store(0.0f);
        channel.rmsReady.store(false);
    }
    publishedChannelCount.store(0);
    audioThreadGeneration = generation;
    publishedGeneration.store(generation);
    publicationSequence.store(++audioThreadSequence);
    hadAudio = false;
}

void MeteredAudioSource::getNextAudioBlock(const juce::AudioSourceChannelInfo& info)
{
    const auto generation = requestedGeneration.load();
    if (generation != audioThreadGeneration)
        clearAudioThreadState(generation);

    const bool wasPlaying = transport.isPlaying();
    transport.getNextAudioBlock(info);

    // Pause, stop, and end-of-file must not leave old readings visible.
    if (!wasPlaying || !transport.isPlaying())
    {
        if (hadAudio)
            clearAudioThreadState(generation);
        return;
    }

    const int channels = juce::jmin(info.buffer->getNumChannels(), 2);
    // Only clear a peak once the UI has acknowledged this exact publication.
    // If another block arrives during a UI read, retain the peak for the next read.
    // This avoids retry loops and locks on the audio thread.
    if (consumedSequence.load() == audioThreadSequence)
        pendingPeaks.fill(0.0f);
    for (int channel = 0; channel < channels; ++channel)
    {
        const std::span<const float> samples(
            info.buffer->getReadPointer(channel, info.startSample),
            static_cast<std::size_t>(info.numSamples));
        const auto levels = meters[static_cast<std::size_t>(channel)].processSamples(samples);
        auto& destination = published[static_cast<std::size_t>(channel)];
        auto& pendingPeak = pendingPeaks[static_cast<std::size_t>(channel)];
        pendingPeak = juce::jmax(pendingPeak, levels.peak);
        destination.peak.store(pendingPeak);
        destination.rms.store(levels.rms);
        destination.rmsReady.store(levels.rmsReady);
    }
    publishedChannelCount.store(channels);
    publishedGeneration.store(generation);
    publicationSequence.store(++audioThreadSequence);
    hadAudio = true;
}

void MeteredAudioSource::requestReset()
{
    // Old publications are ignored immediately; reset runs on the next audio callback.
    requestedGeneration.fetch_add(1);
}

LivePlaybackLevels MeteredAudioSource::consumeLevels()
{
    LivePlaybackLevels levels;
    const auto generation = requestedGeneration.load();
    if (!transport.isPlaying() || publishedGeneration.load() != generation)
        return levels;

    const auto sequence = publicationSequence.load();
    levels.channelCount = publishedChannelCount.load();
    for (int channel = 0; channel < levels.channelCount; ++channel)
    {
        const auto& source = published[static_cast<std::size_t>(channel)];
        auto& destination = levels.channels[static_cast<std::size_t>(channel)];
        destination.rmsReady = source.rmsReady.load();
        destination.peak = source.peak.load();
        destination.rms = source.rms.load();
    }
    if (requestedGeneration.load() != generation || publishedGeneration.load() != generation)
        return {};
    consumedSequence.store(sequence);
    return levels;
}
}
