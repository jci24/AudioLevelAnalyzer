#include "Audio/SoundPressureLevel.h"
#include "TestAssertions.h"

#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace
{
bool referencePressureIsZeroDbSpl()
{
    // Arrange
    const double rms = 0.000020;
    const double pascalsPerSampleUnit = 1.0;
    const double expected = 0.0;

    // Act
    const double actual = audio::rmsToDbSpl(rms, pascalsPerSampleUnit);

    // Assert
    return tests::expectNear("reference pressure is 0 dB SPL", actual, expected);
}

bool onePascalHasKnownSpl()
{
    // Arrange
    const double rms = 1.0;
    const double pascalsPerSampleUnit = 1.0;
    const double expected = 93.97940008672037;

    // Act
    const double actual = audio::rmsToDbSpl(rms, pascalsPerSampleUnit);

    // Assert
    return tests::expectNear("1 Pa RMS is about 93.98 dB SPL", actual, expected);
}

bool oneTenthPascalHasKnownSpl()
{
    // Arrange
    const double rms = 0.1;
    const double pascalsPerSampleUnit = 1.0;
    const double expected = 73.97940008672037;

    // Act
    const double actual = audio::rmsToDbSpl(rms, pascalsPerSampleUnit);

    // Assert
    return tests::expectNear("0.1 Pa RMS is about 73.98 dB SPL", actual, expected);
}

bool calibrationConvertsSampleUnitsToPascals()
{
    // Arrange
    const double rms = 0.5;
    const double pascalsPerSampleUnit = 2.0;
    const double expected = 93.97940008672037;

    // Act
    const double actual = audio::rmsToDbSpl(rms, pascalsPerSampleUnit);

    // Assert
    return tests::expectNear("calibration converts 0.5 sample units to 1 Pa", actual, expected);
}

bool belowReferenceCanHaveNegativeSpl()
{
    // Arrange
    const double rms = 0.000002;
    const double pascalsPerSampleUnit = 1.0;
    const double expected = -20.0;

    // Act
    const double actual = audio::rmsToDbSpl(rms, pascalsPerSampleUnit);

    // Assert
    return tests::expectNear("pressure below the reference has negative dB SPL", actual, expected);
}

bool doublingPressureAddsSixDb()
{
    // Arrange
    const double rms = 0.1;
    const double calibration = 1.0;
    const double expectedIncrease = 6.020599913279624;

    // Act
    const double originalSpl = audio::rmsToDbSpl(rms, calibration);
    const double doubledSpl = audio::rmsToDbSpl(2.0 * rms, calibration);

    // Assert
    return tests::expectNear("doubling pressure adds about 6.02 dB",
                             doubledSpl - originalSpl, expectedIncrease);
}

bool silenceReturnsNegativeInfinity()
{
    // Arrange
    const double rms = 0.0;
    const double calibration = 1.0;

    // Act
    const double actual = audio::rmsToDbSpl(rms, calibration);

    // Assert
    return tests::expectTrue("silence returns negative infinity",
                             std::isinf(actual) && actual < 0.0);
}

bool invalidInputsAreRejected()
{
    // Arrange: each pair contains RMS and calibration, with one invalid value.
    const double infinity = std::numeric_limits<double>::infinity();
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const std::array invalidInputs {
        std::array { -0.1, 1.0 },
        std::array { infinity, 1.0 },
        std::array { nan, 1.0 },
        std::array { 0.1, 0.0 },
        std::array { 0.1, -1.0 },
        std::array { 0.1, infinity },
        std::array { 0.1, nan },
        std::array { 0.0, 0.0 }
    };
    int rejectedInputs = 0;

    // Act
    for (const auto& input : invalidInputs)
    {
        try
        {
            audio::rmsToDbSpl(input[0], input[1]);
        }
        catch (const std::invalid_argument&)
        {
            ++rejectedInputs;
        }
    }

    // Assert
    return tests::expectTrue("invalid RMS/calibration values are rejected",
                             rejectedInputs == static_cast<int>(invalidInputs.size()));
}
}

int main()
{
    const std::array results {
        referencePressureIsZeroDbSpl(),
        onePascalHasKnownSpl(),
        oneTenthPascalHasKnownSpl(),
        calibrationConvertsSampleUnitsToPascals(),
        belowReferenceCanHaveNegativeSpl(),
        doublingPressureAddsSixDb(),
        silenceReturnsNegativeInfinity(),
        invalidInputsAreRejected()
    };

    for (const bool passed : results)
    {
        if (!passed)
        {
            return 1;
        }
    }

    return 0;
}
