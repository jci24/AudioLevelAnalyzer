#include "AudioFileAnalysis.h"
#include "Audio/SamplePeak.h"
#include "Audio/SampleRms.h"

#include <juce_audio_formats/juce_audio_formats.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <numeric>
#include <span>

namespace audio
{
namespace
{
constexpr int chunkSize = 4096;
constexpr unsigned int maximumChannels = 256;

// Unlike RMS, sums of squares can be added across chunks of different lengths.
struct ChannelMeasurements
{
    float peak = 0.0f;
    double sumOfSquares = 0.0;
};

struct FileScan
{
    std::vector<ChannelMeasurements> channelTotals;
    std::string error;
    bool cancelled = false;
};

// Pure calculations: read inputs and return new values, without file access.
bool containsOnlyFiniteSamples(std::span<const float> samples)
{
    return std::all_of(samples.begin(), samples.end(), [](float sample)
    {
        return std::isfinite(sample);
    });
}

ChannelMeasurements measureChannelChunk(std::span<const float> samples)
{
    return { findSamplePeak(samples), sumSquaredSamples(samples) };
}

ChannelMeasurements combineMeasurements(const ChannelMeasurements& first,
                                         const ChannelMeasurements& second)
{
    return {
        std::max(first.peak, second.peak),
        first.sumOfSquares + second.sumOfSquares
    };
}

ChannelLevels calculateLevels(const ChannelMeasurements& measurements, double sampleCount)
{
    return { measurements.peak, std::sqrt(measurements.sumOfSquares / sampleCount) };
}

FileAnalysis summariseFileMeasurements(const FileScan& scan,
                                       std::int64_t samplesPerChannel, double sampleRate)
{
    FileAnalysis fileAnalysis;
    fileAnalysis.samplesPerChannel = samplesPerChannel;
    fileAnalysis.sampleRate = sampleRate;
    fileAnalysis.error = scan.error;
    fileAnalysis.cancelled = scan.cancelled;
    fileAnalysis.channels.resize(scan.channelTotals.size());

    // Only completed scans become measurements for the UI.
    if (!scan.error.empty() || scan.cancelled)
        return fileAnalysis;

    const double channelSampleCount = static_cast<double>(samplesPerChannel);

    // Map: turn each channel's accumulated measurements into its peak and RMS.
    std::transform(scan.channelTotals.begin(), scan.channelTotals.end(),
                   fileAnalysis.channels.begin(), [channelSampleCount](const auto& measurements)
    {
        return calculateLevels(measurements, channelSampleCount);
    });

    // Reduce: combine all channels without mixing their waveforms together.
    const auto allChannels = std::accumulate(scan.channelTotals.begin(), scan.channelTotals.end(),
                                             ChannelMeasurements {}, combineMeasurements);
    const double totalSampleCount = channelSampleCount * static_cast<double>(scan.channelTotals.size());
    fileAnalysis.combined = calculateLevels(allChannels, totalSampleCount);
    return fileAnalysis;
}

FileAnalysis failedAnalysis(const std::string& error)
{
    FileAnalysis fileAnalysis;
    fileAnalysis.error = error;
    return fileAnalysis;
}

// File access: JUCE handles recognising and decoding the file format.
std::unique_ptr<juce::AudioFormatReader> openAudioFile(const std::string& absolutePath)
{
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    const juce::File file(juce::String::fromUTF8(absolutePath.c_str()));
    return std::unique_ptr<juce::AudioFormatReader>(formats.createReaderFor(file));
}

bool hasSupportedMetadata(const juce::AudioFormatReader& reader)
{
    return reader.lengthInSamples > 0
        && reader.numChannels > 0 && reader.numChannels <= maximumChannels
        && std::isfinite(reader.sampleRate) && reader.sampleRate > 0.0;
}

FileScan readAndMeasureChannels(juce::AudioFormatReader& reader, std::stop_token stop)
{
    FileScan scan;
    scan.channelTotals.resize(reader.numChannels);
    const int channelCount = static_cast<int>(reader.numChannels);
    juce::AudioBuffer<float> buffer(channelCount, chunkSize);

    for (juce::int64 position = 0; position < reader.lengthInSamples;)
    {
        if (stop.stop_requested())
        {
            scan.cancelled = true;
            return scan;
        }

        // The final chunk can be shorter; never count its unused buffer space.
        const int samplesToRead = static_cast<int>(std::min<juce::int64>(
            chunkSize, reader.lengthInSamples - position));
        if (!reader.read(buffer.getArrayOfWritePointers(), channelCount, position, samplesToRead))
        {
            scan.error = "Reading the audio failed. No partial result will be displayed.";
            return scan;
        }

        for (int channel = 0; channel < channelCount; ++channel)
        {
            const std::span<const float> samples(buffer.getReadPointer(channel),
                                                 static_cast<std::size_t>(samplesToRead));
            if (!containsOnlyFiniteSamples(samples))
            {
                scan.error = "This file contains non-finite audio samples and cannot be analysed.";
                return scan;
            }

            // Measure this chunk, then merge its values into this channel's running totals.
            const auto chunkMeasurements = measureChannelChunk(samples);
            auto& channelTotal = scan.channelTotals[static_cast<std::size_t>(channel)];
            channelTotal = combineMeasurements(channelTotal, chunkMeasurements);
        }
        position += samplesToRead;
    }
    return scan;
}
}

std::string audioFileWildcard()
{
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    return formats.getWildcardForAllFormats().toStdString();
}

FileAnalysis analyseAudioFile(const std::string& absolutePath, std::stop_token stop)
{
    const auto reader = openAudioFile(absolutePath);
    if (reader == nullptr)
        return failedAnalysis("Could not open this audio file. Choose a supported, readable file.");

    if (!hasSupportedMetadata(*reader))
        return failedAnalysis("This file is empty or has unsupported audio metadata (maximum 256 channels).");

    const auto scan = readAndMeasureChannels(*reader, stop);
    return summariseFileMeasurements(scan, reader->lengthInSamples, reader->sampleRate);
}
}
