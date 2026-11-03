#pragma once

#include <span>

namespace audio
{
// Largest absolute sample value in one channel of finite floating-point audio.
// An empty buffer returns zero. Values above 1.0 are preserved, not clipped.
// This function reads the samples without changing them or retaining state.
float findSamplePeak(std::span<const float> samples);
}
