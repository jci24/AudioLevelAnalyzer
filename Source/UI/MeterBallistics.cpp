#include "MeterBallistics.h"

#include <algorithm>
#include <cmath>

namespace metering
{
namespace
{
double boundedLevel(double db)
{
    if (std::isnan(db))
        return MeterBallistics::floorDb;
    return std::clamp(db, MeterBallistics::floorDb, MeterBallistics::ceilingDb);
}
}

void MeterBallistics::reset()
{
    displayedPeak = floorDb;
    displayedRms = floorDb;
    heldPeak = floorDb;
    holdTimeRemaining = 0.0;
}

void MeterBallistics::update(double peakDb, double rmsDb, double elapsedSeconds)
{
    const double elapsed = std::isfinite(elapsedSeconds) ? std::max(0.0, elapsedSeconds) : 0.0;
    const double peak = boundedLevel(peakDb);
    const double rms = boundedLevel(rmsDb);

    // Instant rise, time-based fall: independent of UI frame rate.
    displayedPeak = std::max(peak, displayedPeak - releaseDbPerSecond * elapsed);
    displayedRms = std::max(rms, displayedRms - releaseDbPerSecond * elapsed);

    if (peak >= heldPeak)
    {
        heldPeak = peak;
        holdTimeRemaining = holdSeconds;
    }
    else
    {
        // If a frame crosses the hold deadline, only decay for time after it.
        const double decaySeconds = std::max(0.0, elapsed - holdTimeRemaining);
        holdTimeRemaining = std::max(0.0, holdTimeRemaining - elapsed);
        heldPeak = std::max(peak, heldPeak - holdReleaseDbPerSecond * decaySeconds);
    }
}
}
