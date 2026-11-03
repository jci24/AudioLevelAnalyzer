#include "SampleRms.h"

#include <cmath>

namespace audio
{
double sumSquaredSamples(std::span<const float> samples)
{
    double sumOfSquares = 0.0;

    for (const float sample : samples)
    {
        const double value = static_cast<double>(sample);
        sumOfSquares += value * value;
    }

    return sumOfSquares;
}

float findSampleRms(std::span<const float> samples)
{
    if (samples.empty())
    {
        return 0.0f;
    }

    const double sumOfSquares = sumSquaredSamples(samples);
    const double meanSquare = sumOfSquares / static_cast<double>(samples.size());
    const double rms = std::sqrt(meanSquare);

    return static_cast<float>(rms);
}
}
