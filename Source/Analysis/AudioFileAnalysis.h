#pragma once

#include <cstdint>
#include <stop_token>
#include <string>
#include <vector>

namespace audio
{
struct ChannelLevels
{
    float peak = 0.0f;
    double rms = 0.0;
};

struct FileAnalysis
{
    std::vector<ChannelLevels> channels;
    ChannelLevels combined;
    std::int64_t samplesPerChannel = 0;
    double sampleRate = 0.0;
    std::string error;
    bool cancelled = false;
};

// Read the complete file in bounded chunks. RMS pools every channel's energy;
// peak is the largest sample magnitude, without mixing channels together.
FileAnalysis analyseAudioFile(const std::string& absolutePath, std::stop_token stop = {});
std::string audioFileWildcard();
}
