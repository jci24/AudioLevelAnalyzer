#include "LiveLevelMeter.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace audio
{
void LiveLevelMeter::prepare(double sampleRate)
{
    // Invalid rates disable measurement; never throw from audio preparation.
    windowLength = 0;
    if (std::isfinite(sampleRate) && sampleRate > 0.0)
    {
        const double samplesIn100ms = std::round(sampleRate / 10.0);
        windowLength = static_cast<int>(std::clamp(samplesIn100ms, 1.0,
            static_cast<double>(std::numeric_limits<int>::max())));
    }
    reset();
}

void LiveLevelMeter::reset()
{
    samplesInWindow = 0;
    sumOfSquares = 0.0;
    latestRms = 0.0f;
    rmsReady = false;
}

LiveChannelLevels LiveLevelMeter::processSamples(std::span<const float> samples)
{
    if (windowLength == 0)
        return {};

    float blockPeak = 0.0f;
    for (const float sample : samples)
    {
        blockPeak = std::max(blockPeak, std::abs(sample));
        const double value = sample;
        sumOfSquares += value * value;
        ++samplesInWindow;

        if (samplesInWindow == windowLength)
        {
            latestRms = static_cast<float>(std::sqrt(sumOfSquares / windowLength));
            rmsReady = true;
            samplesInWindow = 0;
            sumOfSquares = 0.0;
        }
    }
    return { blockPeak, latestRms, rmsReady };
}
}
