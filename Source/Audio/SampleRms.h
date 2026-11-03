#pragma once

#include <span>

namespace audio
{
// Add squared finite sample values in double precision; empty buffers return zero.
// These totals can be combined across chunks before calculating whole-file RMS.
double sumSquaredSamples(std::span<const float> samples);

// Root mean square of one channel of finite floating-point audio.
// Empty buffers return zero. Samples are not changed and values are not clipped.
// Each call measures only the supplied buffer, with no retained state.
float findSampleRms(std::span<const float> samples);
}
