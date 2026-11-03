#include "Playback/MeteredAudioSource.h"
#include "TestAssertions.h"

#include <array>
#include <algorithm>

namespace
{
bool retainsPeaksBetweenUiReads()
{
    // Arrange: one brief spike followed by several silent blocks.
    juce::AudioBuffer<float> input(1, 48000);
    input.clear();
    input.setSample(0, 200, -0.9f);
    juce::MemoryAudioSource source(input, false, false);
    juce::AudioTransportSource transport;
    audio::MeteredAudioSource metered(transport);
    transport.setSource(&source);
    metered.prepareToPlay(512, 48000.0);
    juce::AudioBuffer<float> output(1, 512);
    const juce::AudioSourceChannelInfo block(&output, 0, 512);

    // Act: the UI skips three blocks, then consumes the interval peak.
    transport.start();
    for (int index = 0; index < 4; ++index)
        metered.getNextAudioBlock(block);
    const auto captured = metered.consumeLevels();
    metered.getNextAudioBlock(block);
    const auto nextInterval = metered.consumeLevels();

    // Assert: the spike survives quiet blocks, but not its acknowledgement.
    const bool capturedSpike = tests::expectNear("short spike survives between UI reads",
                                                captured.channels[0].peak, 0.9);
    const bool clearedAfterRead = tests::expectNear("acknowledged peak does not remain latched",
                                                   nextInterval.channels[0].peak, 0.0);
    for (int index = 0; index < 120 && transport.isPlaying(); ++index)
        metered.getNextAudioBlock(block);
    metered.releaseResources();
    transport.setSource(nullptr);
    return capturedSpike && clearedAfterRead;
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI initialiseJuce;

    // Arrange: one second of two distinct channels at 24 kHz, rendered at 48 kHz.
    // This exercises the real transport/resampler without opening an audio device.
    juce::AudioBuffer<float> input(2, 24000);
    for (int sample = 0; sample < input.getNumSamples(); ++sample)
    {
        input.setSample(0, sample, 0.25f);
        input.setSample(1, sample, -0.5f);
    }
    juce::MemoryAudioSource source(input, false, false);
    juce::AudioTransportSource transport;
    audio::MeteredAudioSource metered(transport);
    transport.setSource(&source, 0, nullptr, 24000.0);
    metered.prepareToPlay(512, 48000.0);
    juce::AudioBuffer<float> output(2, 540);
    const juce::AudioSourceChannelInfo block(&output, 13, 512);
    std::array<bool, 10> passed {};
    float renderedPeak = 0.0f;

    // Act: collect the actual interval peak, including the resampling startup edge.
    transport.start();
    for (int index = 0; index < 24; ++index)
    {
        output.clear();
        output.setSample(0, 0, 10.0f); // Outside the active output region: must be ignored.
        metered.getNextAudioBlock(block);
        for (int sample = block.startSample; sample < block.startSample + block.numSamples; ++sample)
            renderedPeak = std::max(renderedPeak, std::abs(output.getSample(0, sample)));
    }
    const auto levels = metered.consumeLevels();

    // Assert: separate channels measured after sample-rate conversion.
    passed[0] = tests::expectTrue("tap publishes stereo output", levels.channelCount == 2);
    passed[1] = tests::expectNear("interval peak matches rendered audio", levels.channels[0].peak, renderedPeak);
    passed[2] = tests::expectNear("left output RMS", levels.channels[0].rms, 0.25);
    passed[3] = tests::expectNear("right output RMS", levels.channels[1].rms, 0.5);
    metered.getNextAudioBlock(block);
    const auto steady = metered.consumeLevels();
    passed[9] = tests::expectNear("next interval has the steady peak", steady.channels[0].peak, 0.25);

    // Arrange / Act: reset during playback, then render less than a fresh RMS window.
    metered.requestReset();
    const auto immediatelyAfterReset = metered.consumeLevels();
    metered.getNextAudioBlock(block);
    const auto freshWindow = metered.consumeLevels();

    // Assert: previous readings and partial-window energy cannot cross a reset.
    passed[4] = tests::expectTrue("reset hides old publications immediately",
                                 immediatelyAfterReset.channelCount == 0);
    passed[5] = tests::expectTrue("reset waits for fresh RMS",
                                 freshWindow.channelCount == 2 && !freshWindow.channels[0].rmsReady);

    // Arrange / Act: play through the end and attempt to restart with a fresh generation.
    for (int index = 0; index < 120 && transport.isPlaying(); ++index)
        metered.getNextAudioBlock(block);
    const auto finished = metered.consumeLevels();
    passed[6] = tests::expectTrue("end clears live readings",
                                 !transport.isPlaying() && finished.channelCount == 0);
    transport.setPosition(0.0);
    metered.requestReset();
    transport.start();
    metered.getNextAudioBlock(block);
    const auto restarted = metered.consumeLevels();
    passed[7] = tests::expectTrue("restart does not reuse previous RMS",
                                 restarted.channelCount == 2 && !restarted.channels[0].rmsReady);

    // Finish rendering before releasing the transport (no device callback runs in this test).
    for (int index = 0; index < 120 && transport.isPlaying(); ++index)
        metered.getNextAudioBlock(block);
    metered.releaseResources();
    transport.setSource(nullptr);
    passed[8] = retainsPeaksBetweenUiReads();
    return std::all_of(passed.begin(), passed.end(), [](bool value) { return value; }) ? 0 : 1;
}
