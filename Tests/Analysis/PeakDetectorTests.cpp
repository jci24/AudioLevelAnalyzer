#include "Analysis/SamplePeak.h"
#include "Support/TestAssertions.h"

#include <array>

namespace
{
bool emptyBufferReturnsZero()
{
    // Arrange: prepare the input and expected result.
    const std::array<float, 0> samples {};
    const float expected = 0.0f;

    // Act: run the detector.
    const float actual = audio::findSamplePeak(samples);

    // Assert: compare the result with our expectation.
    return tests::expectNear("empty buffer", actual, expected);
}

bool silenceReturnsZero()
{
    // Arrange: prepare the input and expected result.
    const std::array samples { 0.0f, 0.0f, 0.0f };
    const float expected = 0.0f;

    // Act: run the detector.
    const float actual = audio::findSamplePeak(samples);

    // Assert: compare the result with our expectation.
    return tests::expectNear("silence", actual, expected);
}

bool negativePeakUsesMagnitude()
{
    // Arrange: prepare the input and expected result.
    const std::array samples { 0.1f, -0.8f, 0.3f };
    const float expected = 0.8f;

    // Act: run the detector.
    const float actual = audio::findSamplePeak(samples);

    // Assert: compare the result with our expectation.
    return tests::expectNear("negative peak", actual, expected);
}

bool positivePeakIsFound()
{
    // Arrange: prepare the input and expected result.
    const std::array samples { -0.2f, 0.9f, -0.4f };
    const float expected = 0.9f;

    // Act: run the detector.
    const float actual = audio::findSamplePeak(samples);

    // Assert: compare the result with our expectation.
    return tests::expectNear("positive peak", actual, expected);
}

bool firstSampleCanBeThePeak()
{
    // Arrange: prepare the input and expected result.
    const std::array samples { -0.9f, 0.2f, 0.1f };
    const float expected = 0.9f;

    // Act: run the detector.
    const float actual = audio::findSamplePeak(samples);

    // Assert: compare the result with our expectation.
    return tests::expectNear("peak at first sample", actual, expected);
}

bool lastSampleCanBeThePeak()
{
    // Arrange: prepare the input and expected result.
    const std::array samples { 0.1f, 0.2f, -0.9f };
    const float expected = 0.9f;

    // Act: run the detector.
    const float actual = audio::findSamplePeak(samples);

    // Assert: compare the result with our expectation.
    return tests::expectNear("peak at last sample", actual, expected);
}

bool singleSampleUsesMagnitude()
{
    // Arrange: prepare the input and expected result.
    const std::array samples { -0.4f };
    const float expected = 0.4f;

    // Act: run the detector.
    const float actual = audio::findSamplePeak(samples);

    // Assert: compare the result with our expectation.
    return tests::expectNear("single sample", actual, expected);
}

bool valuesAboveOneAreNotClipped()
{
    // Arrange: prepare the input and expected result.
    const std::array samples { 0.5f, -1.2f, 1.1f };
    const float expected = 1.2f;

    // Act: run the detector.
    const float actual = audio::findSamplePeak(samples);

    // Assert: compare the result with our expectation.
    return tests::expectNear("above nominal full scale", actual, expected);
}

bool previousBufferDoesNotAffectNextMeasurement()
{
    // Arrange
    const std::array louderSamples { 0.1f, -0.8f, 0.3f };
    const std::array quieterSamples { 0.1f, -0.2f };
    const float expectedLouder = 0.8f;
    const float expectedQuieter = 0.2f;

    // Act
    const float actualLouder = audio::findSamplePeak(louderSamples);
    const float actualQuieter = audio::findSamplePeak(quieterSamples);

    // Assert
    const bool louderPassed = tests::expectNear("first buffer", actualLouder, expectedLouder);
    const bool quieterPassed = tests::expectNear("independent quieter buffer", actualQuieter, expectedQuieter);
    return louderPassed && quieterPassed;
}

bool measurementDoesNotChangeSamples()
{
    // Arrange
    const std::array original { 0.1f, -0.8f, 0.3f };
    auto samples = original;
    const float expected = 0.8f;

    // Act
    const float actual = audio::findSamplePeak(samples);

    // Assert
    const bool resultPassed = tests::expectNear("read-only measurement result", actual, expected);
    const bool samplesUnchanged = tests::expectTrue("input samples unchanged", samples == original);
    return resultPassed && samplesUnchanged;
}
}

int main()
{
    // Run every test, then return a failure status if any did not pass.
    const std::array results {
        emptyBufferReturnsZero(),
        silenceReturnsZero(),
        negativePeakUsesMagnitude(),
        positivePeakIsFound(),
        firstSampleCanBeThePeak(),
        lastSampleCanBeThePeak(),
        singleSampleUsesMagnitude(),
        valuesAboveOneAreNotClipped(),
        previousBufferDoesNotAffectNextMeasurement(),
        measurementDoesNotChangeSamples()
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
