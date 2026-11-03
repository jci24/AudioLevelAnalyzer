#pragma once

#include <span>

namespace audio
{
struct LiveChannelLevels
{
    float peak = 0.0f;
    float rms = 0.0f;
    bool rmsReady = false;
};

// One channel, owned by the audio thread. RMS uses consecutive 100 ms windows,
// independent of device block size. No allocation, file access, or UI calls.
class LiveLevelMeter
{
public:
    void prepare(double sampleRate);
    void reset();
    LiveChannelLevels processSamples(std::span<const float> samples);

private:
    int windowLength = 0;
    int samplesInWindow = 0;
    double sumOfSquares = 0.0;
    float latestRms = 0.0f;
    bool rmsReady = false;
};
}
