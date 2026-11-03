#include "Analysis/SamplePeak.h"
#include "Analysis/SampleRms.h"
#include "Analysis/SoundPressureLevel.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <numbers>

int main()
{
    constexpr double sampleRate = 48000.0;
    constexpr double frequency = 1000.0;
    constexpr float amplitude = 0.5f;
    constexpr std::size_t bufferSize = 512;
    // Learning assumption, not a measured microphone calibration.
    constexpr double pascalsPerSampleUnit = 1.0;

    std::array<float, bufferSize> samples {};

    for (std::size_t index = 0; index < samples.size(); ++index)
    {
        const double timeInSeconds = static_cast<double>(index) / sampleRate;
        const double angle = 2.0 * std::numbers::pi * frequency * timeInSeconds;
        samples[index] = amplitude * static_cast<float>(std::sin(angle));
    }

    const float peak = audio::findSamplePeak(samples);
    const float rms = audio::findSampleRms(samples);
    const double spl = audio::rmsToDbSpl(rms, pascalsPerSampleUnit);
    std::cout << "Measured peak: " << peak << '\n';
    std::cout << "Measured RMS: " << rms << '\n';
    std::cout << "Assumed calibration: " << pascalsPerSampleUnit << " Pa per sample unit\n";
    std::cout << "Simulated SPL (unweighted RMS): " << spl << " dB SPL\n";
}
