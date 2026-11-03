#pragma once

namespace metering
{
// UI-owned visual state. It never changes the measured samples or numeric RMS.
class MeterBallistics
{
public:
    static constexpr double floorDb = -60.0;
    static constexpr double ceilingDb = 6.0;
    static constexpr double releaseDbPerSecond = 24.0;
    static constexpr double holdSeconds = 1.0;
    static constexpr double holdReleaseDbPerSecond = 12.0;

    void reset();
    void update(double peakDb, double rmsDb, double elapsedSeconds);
    double peakBarDb() const { return displayedPeak; }
    double rmsBarDb() const { return displayedRms; }
    double heldPeakDb() const { return heldPeak; }

private:
    double displayedPeak = floorDb;
    double displayedRms = floorDb;
    double heldPeak = floorDb;
    double holdTimeRemaining = 0.0;
};
}
