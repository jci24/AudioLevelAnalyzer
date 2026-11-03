#pragma once

namespace audio
{
// Unweighted RMS sound pressure level relative to 20 micropascals (in air).
// The calibration must map one sample unit to a pressure in pascals.
// Without measured calibration, the result represents an assumed/simulated SPL.
// Zero RMS returns negative infinity. Invalid inputs throw std::invalid_argument:
// RMS must be finite and nonnegative; calibration must be finite and positive.
double rmsToDbSpl(double rms, double pascalsPerSampleUnit);
}
