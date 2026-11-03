#include "Analysis/SampleRms.h"
#include "Support/TestAssertions.h"

#include <array>
#include <cmath>
#include <limits>

namespace
{
bool emptyBufferReturnsZero()
{
    // Arrange: prepare the input and expected result.
    const std::array<float, 0> samples {};
    const float expected = 0.0f;

    // Act: run the detector.
    const float actual = audio::findSampleRms(samples);

    // Assert: compare the result with our expectation.
    return tests::expectNear("empty buffer", actual, expected);
}

bool silenceReturnsZero()
{
    // Arrange: prepare the input and expected result.
    const std::array samples { 0.0f, 0.0f, 0.0f };
    const float expected = 0.0f;

    // Act: run the detector.
    const float actual = audio::findSampleRms(samples);

    // Assert: compare the result with our expectation.
    return tests::expectNear("silence", actual, expected);
}

bool singleNegativeSampleUsesMagnitude()
{
    // Arrange: prepare the input and expected result.
    const std::array samples { -0.4f };
    const float expected = 0.4f;

    // Act: run the detector.
    const float actual = audio::findSampleRms(samples);

    // Assert: compare the result with our expectation.
    return tests::expectNear("single negative sample", actual, expected);
}

bool constantMagnitudeIsPreserved()
{
    // Arrange: prepare the input and expected result.
    const std::array samples { 0.5f, -0.5f, 0.5f, -0.5f };
    const float expected = 0.5f;

    // Act: run the detector.
    const float actual = audio::findSampleRms(samples);

    // Assert: compare the result with our expectation.
    return tests::expectNear("constant magnitude", actual, expected);
}

bool isolatedSpikeHasLowerRms()
{
    // Arrange: prepare the input and expected result.
    const std::array samples { 1.0f, 0.0f, 0.0f, 0.0f };
    const float expected = 0.5f;

    // Act: run the detector.
    const float actual = audio::findSampleRms(samples);

    // Assert: compare the result with our expectation.
    return tests::expectNear("one spike among zeros", actual, expected);
}

bool sustainedFullScaleHasRmsOfOne()
{
    // Arrange: prepare the input and expected result.
    const std::array samples { 1.0f, -1.0f, 1.0f, -1.0f };
    const float expected = 1.0f;

    // Act: run the detector.
    const float actual = audio::findSampleRms(samples);

    // Assert: compare the result with our expectation.
    return tests::expectNear("sustained full scale", actual, expected);
}

bool fourPointSineHasExpectedRms()
{
    // Arrange: prepare the input and expected result.
    const std::array samples { 0.0f, 0.5f, 0.0f, -0.5f };
    const float expected = static_cast<float>(std::sqrt(0.125));

    // Act: run the detector.
    const float actual = audio::findSampleRms(samples);

    // Assert: compare the result with our expectation.
    return tests::expectNear("four-point sine cycle", actual, expected);
}

bool valuesAboveOneAreNotClipped()
{
    // Arrange: prepare the input and expected result.
    const std::array samples { -1.2f, 1.2f };
    const float expected = 1.2f;

    // Act: run the detector.
    const float actual = audio::findSampleRms(samples);

    // Assert: compare the result with our expectation.
    return tests::expectNear("above nominal full scale", actual, expected);
}

bool largeSamplesDoNotOverflowDuringSquaring()
{
    // Arrange: prepare the input and expected result.
    const std::array samples { std::numeric_limits<float>::max() };
    const float expected = std::numeric_limits<float>::max();

    // Act: run the detector.
    const float actual = audio::findSampleRms(samples);

    // Assert: compare the result with our expectation.
    return tests::expectNear("large values do not overflow during squaring", actual, expected);
}

bool quietSamplesRemainMeasurable()
{
    // Arrange: prepare the input and expected result.
    const std::array samples { 1.0e-30f, -1.0e-30f };
    const float expected = 1.0e-30f;

    // Act: run the detector.
    const float actual = audio::findSampleRms(samples);

    // Assert: compare the result with our expectation.
    return tests::expectNear("quiet values remain measurable", actual, expected);
}

bool previousBufferDoesNotAffectNextMeasurement()
{
    // Arrange
    const std::array louderSamples { 0.5f, -0.5f };
    const std::array quieterSamples { 0.1f, -0.1f };
    const float expectedLouder = 0.5f;
    const float expectedQuieter = 0.1f;

    // Act
    const float actualLouder = audio::findSampleRms(louderSamples);
    const float actualQuieter = audio::findSampleRms(quieterSamples);

    // Assert
    const bool louderPassed = tests::expectNear("first buffer", actualLouder, expectedLouder);
    const bool quieterPassed = tests::expectNear("independent quieter buffer", actualQuieter, expectedQuieter);
    return louderPassed && quieterPassed;
}

bool measurementDoesNotChangeSamples()
{
    // Arrange
    const std::array original { 0.5f, -0.5f };
    auto samples = original;
    const float expected = 0.5f;

    // Act
    const float actual = audio::findSampleRms(samples);

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
        singleNegativeSampleUsesMagnitude(),
        constantMagnitudeIsPreserved(),
        isolatedSpikeHasLowerRms(),
        sustainedFullScaleHasRmsOfOne(),
        fourPointSineHasExpectedRms(),
        valuesAboveOneAreNotClipped(),
        largeSamplesDoNotOverflowDuringSquaring(),
        quietSamplesRemainMeasurable(),
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
