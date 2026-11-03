#include "SoundPressureLevel.h"

#include <cmath>
#include <limits>
#include <stdexcept>

namespace audio
{
double rmsToDbSpl(double rms, double pascalsPerSampleUnit)
{
    if (!std::isfinite(rms) || rms < 0.0)
    {
        throw std::invalid_argument("RMS must be finite and nonnegative");
    }

    if (!std::isfinite(pascalsPerSampleUnit) || pascalsPerSampleUnit <= 0.0)
    {
        throw std::invalid_argument("Pascals per sample unit must be finite and positive");
    }

    if (rms == 0.0)
    {
        return -std::numeric_limits<double>::infinity();
    }

    constexpr double referencePressure = 0.000020; // 20 micropascals.

    // Equivalent to 20 * log10(rms * calibration / referencePressure).
    // Separate logarithms avoid overflow/underflow in the intermediate product.
    return 20.0 * (std::log10(rms) + std::log10(pascalsPerSampleUnit)
                   - std::log10(referencePressure));
}
}
