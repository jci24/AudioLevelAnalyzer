#include "Metering/LiveLevelMeter.h"
#include "Support/TestAssertions.h"

#include <algorithm>
#include <array>
#include <numbers>
#include <vector>

namespace
{
bool waitsForOneHundredMilliseconds()
{
    // Arrange
    audio::LiveLevelMeter meter;
    meter.prepare(48000.0);
    const std::vector<float> almostAWindow(4799, 0.5f);
    const std::array finalSample { 0.5f };

    // Act
    const auto before = meter.processSamples(almostAWindow);
    const auto complete = meter.processSamples(finalSample);

    // Assert
    return tests::expectTrue("RMS waits for a full 100 ms window", !before.rmsReady && complete.rmsReady)
        && tests::expectNear("block peak", before.peak, 0.5)
        && tests::expectNear("constant magnitude RMS", complete.rms, 0.5);
}

bool sineHasKnownPeakAndRms()
{
    // Arrange
    audio::LiveLevelMeter meter;
    meter.prepare(48000.0);
    std::vector<float> samples(4800);
    for (std::size_t index = 0; index < samples.size(); ++index)
        samples[index] = static_cast<float>(0.5 * std::sin(2.0 * std::numbers::pi * 1000.0
                                                           * static_cast<double>(index) / 48000.0));

    // Act
    const auto levels = meter.processSamples(samples);

    // Assert
    return tests::expectNear("sine peak", levels.peak, 0.5)
        && tests::expectNear("sine RMS", levels.rms, 0.3535533905932738);
}

bool rmsDoesNotDependOnDeviceBlockSize()
{
    // Arrange: a louder window followed by a quieter one, with boundaries inside blocks.
    std::vector<float> samples(9600, 0.1f);
    std::fill_n(samples.begin(), 4800, 0.5f);
    const std::array blockSizes { 64, 512, 4096, 9600 };
    bool passed = true;

    for (const int blockSize : blockSizes)
    {
        audio::LiveLevelMeter meter;
        meter.prepare(48000.0);
        audio::LiveChannelLevels levels;

        // Act
        for (std::size_t position = 0; position < samples.size();)
        {
            const auto count = std::min(static_cast<std::size_t>(blockSize), samples.size() - position);
            levels = meter.processSamples(std::span<const float>(samples.data() + position, count));
            position += count;
        }

        // Assert
        passed = tests::expectNear("100 ms RMS is independent of device block size", levels.rms, 0.1)
            && passed;
    }
    return passed;
}

bool sampleRateDefinesTheWindowLength()
{
    // Arrange
    audio::LiveLevelMeter meter;
    meter.prepare(44100.0);
    const std::vector<float> almostAWindow(4409, 0.25f);
    const std::array finalSample { 0.25f };

    // Act
    const auto before = meter.processSamples(almostAWindow);
    const auto complete = meter.processSamples(finalSample);

    // Assert
    return tests::expectTrue("44.1 kHz needs 4410 samples", !before.rmsReady && complete.rmsReady)
        && tests::expectNear("44.1 kHz RMS", complete.rms, 0.25);
}

bool resetDiscardsThePreviousPartialWindow()
{
    // Arrange
    audio::LiveLevelMeter meter;
    meter.prepare(48000.0);
    const std::vector<float> loudHalfWindow(2400, 0.9f);
    const std::vector<float> quietHalfWindow(2400, 0.1f);
    meter.processSamples(loudHalfWindow);

    // Act
    meter.reset();
    const auto halfway = meter.processSamples(quietHalfWindow);
    const auto complete = meter.processSamples(quietHalfWindow);

    // Assert
    return tests::expectTrue("reset starts a fresh window", !halfway.rmsReady)
        && tests::expectNear("reset removes old energy", complete.rms, 0.1);
}

bool silenceAndNewSampleRateClearOldLevels()
{
    // Arrange
    audio::LiveLevelMeter meter;
    meter.prepare(48000.0);
    const std::vector<float> loudWindow(4800, 0.5f);
    meter.processSamples(loudWindow);

    // Act
    meter.prepare(44100.0);
    const auto fresh = meter.processSamples({});
    const auto silence = meter.processSamples(std::vector<float>(4410, 0.0f));

    // Assert
    return tests::expectTrue("new rate clears readiness", !fresh.rmsReady)
        && tests::expectTrue("silence completes a window", silence.rmsReady)
        && tests::expectNear("silent peak", silence.peak, 0.0)
        && tests::expectNear("silent RMS", silence.rms, 0.0);
}

bool peakTracksEachBlockInsteadOfHoldingTheMaximum()
{
    // Arrange
    audio::LiveLevelMeter meter;
    meter.prepare(48000.0);
    const std::array loud { -0.9f };
    const std::array quiet { 0.1f };
    meter.processSamples(loud);

    // Act
    const auto levels = meter.processSamples(quiet);

    // Assert
    return tests::expectNear("peak describes the current block", levels.peak, 0.1);
}
}

int main()
{
    const std::array results {
        waitsForOneHundredMilliseconds(),
        sineHasKnownPeakAndRms(),
        rmsDoesNotDependOnDeviceBlockSize(),
        sampleRateDefinesTheWindowLength(),
        resetDiscardsThePreviousPartialWindow(),
        silenceAndNewSampleRateClearOldLevels(),
        peakTracksEachBlockInsteadOfHoldingTheMaximum()
    };
    return std::all_of(results.begin(), results.end(), [](bool passed) { return passed; }) ? 0 : 1;
}
