#include "SamplePeak.h"

#include <algorithm>
#include <cmath>

namespace audio
{
float findSamplePeak(std::span<const float> samples)
{
    float peak = 0.0f;

    for (const float sample : samples)
    {
        const float magnitude = std::abs(sample);

        if (magnitude > peak)
        {
            peak = magnitude;
        }
    }

    return peak;
}
}
