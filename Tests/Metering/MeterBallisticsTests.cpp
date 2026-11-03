#include "Metering/MeterBallistics.h"
#include "Support/TestAssertions.h"

#include <algorithm>
#include <array>
#include <limits>

namespace
{
bool attacksImmediatelyAndReleasesByElapsedTime()
{
    // Arrange
    metering::MeterBallistics display;

    // Act
    display.update(-6.0, -9.0, 0.0);
    const double attackedPeak = display.peakBarDb();
    display.update(-60.0, -60.0, 0.25);

    // Assert
    return tests::expectNear("instant peak rise", attackedPeak, -6.0)
        && tests::expectNear("peak falls 6 dB in 250 ms", display.peakBarDb(), -12.0)
        && tests::expectNear("RMS falls 6 dB in 250 ms", display.rmsBarDb(), -15.0);
}

bool holdsForOneSecondThenFalls()
{
    // Arrange
    metering::MeterBallistics display;
    display.update(-3.0, -6.0, 0.0);

    // Act
    display.update(-60.0, -60.0, 0.75);
    const double held = display.heldPeakDb();
    display.update(-60.0, -60.0, 0.5); // Cross the deadline: 250 ms of decay.

    // Assert
    return tests::expectNear("peak held before one second", held, -3.0)
        && tests::expectNear("hold decays only after deadline", display.heldPeakDb(), -6.0);
}

bool displayDoesNotDependOnFrameRate()
{
    // Arrange
    metering::MeterBallistics oneFrame;
    metering::MeterBallistics manyFrames;
    oneFrame.update(-3.0, -6.0, 0.0);
    manyFrames.update(-3.0, -6.0, 0.0);

    // Act
    oneFrame.update(-60.0, -60.0, 1.5);
    for (int frame = 0; frame < 45; ++frame)
        manyFrames.update(-60.0, -60.0, 1.0 / 30.0);

    // Assert
    return tests::expectNear("frame-independent peak", oneFrame.peakBarDb(), manyFrames.peakBarDb())
        && tests::expectNear("frame-independent RMS", oneFrame.rmsBarDb(), manyFrames.rmsBarDb())
        && tests::expectNear("frame-independent hold", oneFrame.heldPeakDb(), manyFrames.heldPeakDb());
}

bool higherPeakRestartsHoldAndResetClearsHistory()
{
    // Arrange
    metering::MeterBallistics display;
    display.update(-12.0, -15.0, 0.0);
    display.update(-60.0, -60.0, 0.9);

    // Act
    display.update(-3.0, -6.0, 0.1);
    display.update(-60.0, -60.0, 0.9);
    const double held = display.heldPeakDb();
    display.reset();

    // Assert
    return tests::expectNear("higher peak restarts hold", held, -3.0)
        && tests::expectNear("reset clears peak", display.peakBarDb(), -60.0)
        && tests::expectNear("reset clears RMS", display.rmsBarDb(), -60.0)
        && tests::expectNear("reset clears hold", display.heldPeakDb(), -60.0);
}

bool visualRangeHandlesSilenceAndOverRange()
{
    // Arrange
    metering::MeterBallistics display;

    // Act
    display.update(20.0, 10.0, 0.0);
    const double capped = display.peakBarDb();
    display.update(-std::numeric_limits<double>::infinity(),
                   -std::numeric_limits<double>::infinity(), 10.0);

    // Assert: only the visual range is capped; measurements live elsewhere.
    return tests::expectNear("bar caps at scale ceiling", capped, 6.0)
        && tests::expectNear("silence reaches scale floor", display.peakBarDb(), -60.0)
        && tests::expectNear("held peak eventually clears", display.heldPeakDb(), -60.0);
}
}

int main()
{
    const std::array results {
        attacksImmediatelyAndReleasesByElapsedTime(),
        holdsForOneSecondThenFalls(),
        displayDoesNotDependOnFrameRate(),
        higherPeakRestartsHoldAndResetClearsHistory(),
        visualRangeHandlesSilenceAndOverRange()
    };
    return std::all_of(results.begin(), results.end(), [](bool passed) { return passed; }) ? 0 : 1;
}
