#include "Files/AudioFileAnalysis.h"
#include "Audio/SoundPressureLevel.h"
#include "TestAssertions.h"

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace
{
// Write tiny IEEE-float WAV fixtures, independent of JUCE's audio writer.
void writeUnsigned(std::ofstream& stream, std::uint32_t value, int bytes)
{
    for (int byte = 0; byte < bytes; ++byte)
        stream.put(static_cast<char>((value >> (8 * byte)) & 0xff));
}

void writeWave(const std::filesystem::path& path, const std::vector<float>& interleaved,
               int channels = 1)
{
    std::ofstream stream(path, std::ios::binary);
    const auto dataSize = static_cast<std::uint32_t>(interleaved.size() * sizeof(float));
    stream.write("RIFF", 4);
    writeUnsigned(stream, 36 + dataSize, 4);
    stream.write("WAVEfmt ", 8);
    writeUnsigned(stream, 16, 4);
    writeUnsigned(stream, 3, 2); // IEEE floating-point WAV.
    writeUnsigned(stream, static_cast<std::uint32_t>(channels), 2);
    writeUnsigned(stream, 48000, 4);
    writeUnsigned(stream, static_cast<std::uint32_t>(48000 * channels * 4), 4);
    writeUnsigned(stream, static_cast<std::uint32_t>(channels * 4), 2);
    writeUnsigned(stream, 32, 2);
    stream.write("data", 4);
    writeUnsigned(stream, dataSize, 4);
    for (const float sample : interleaved)
        writeUnsigned(stream, std::bit_cast<std::uint32_t>(sample), 4);
    if (!stream)
        throw std::runtime_error("Could not write test fixture");
}

bool measuresMonoAndPreservesOverFullScale(const std::filesystem::path& folder)
{
    // Arrange
    const auto path = folder / "mono.wav";
    writeWave(path, { 0.0f, -1.5f, 0.0f, 0.0f });

    // Act
    const auto result = audio::analyseAudioFile(path.string());

    // Assert
    return tests::expectTrue("mono metadata", result.error.empty()
        && result.channels.size() == 1 && result.samplesPerChannel == 4 && result.sampleRate == 48000)
        && tests::expectNear("whole-file mono peak", result.combined.peak, 1.5)
        && tests::expectNear("whole-file mono RMS", result.combined.rms, 0.75);
}

bool weightsTheFinalShortChunkCorrectly(const std::filesystem::path& folder)
{
    // Arrange: first chunk is silent, final single sample contains the peak.
    const auto path = folder / "partial-final-chunk.wav";
    std::vector<float> samples(4097, 0.0f);
    samples.back() = -0.5f;
    writeWave(path, samples);
    const double expectedRms = 0.5 / std::sqrt(4097.0);

    // Act
    const auto result = audio::analyseAudioFile(path.string());

    // Assert: averaging the two chunk RMS values or padding the last chunk fails this.
    return tests::expectTrue("partial chunk analysed successfully", result.error.empty())
        && tests::expectNear("peak in final sample", result.combined.peak, 0.5)
        && tests::expectNear("RMS weights actual samples", result.combined.rms, expectedRms);
}

bool retainsPrecisionUntilWholeFileRms(const std::filesystem::path& folder)
{
    // Arrange: exact binary samples with an RMS that does not fit exactly in float.
    const auto path = folder / "rms-precision.wav";
    writeWave(path, { 0.5f, 0.0f, 0.0f });
    const double expectedRms = std::sqrt(1.0 / 12.0);

    // Act
    const auto analysis = audio::analyseAudioFile(path.string());

    // Assert: converting an intermediate chunk RMS to float loses this precision.
    return tests::expectTrue("whole-file RMS retains double precision",
        analysis.error.empty() && std::abs(analysis.combined.rms - expectedRms) < 1.0e-12);
}

bool measuresStereoWithoutCancellation(const std::filesystem::path& folder)
{
    // Arrange: opposite signs must not cancel each other; right channel is quieter.
    const auto path = folder / "stereo.wav";
    writeWave(path, { 0.5f, -0.25f, 0.5f, -0.25f }, 2);

    // Act
    const auto result = audio::analyseAudioFile(path.string());

    // Assert
    return tests::expectTrue("stereo metadata", result.error.empty() && result.channels.size() == 2)
        && tests::expectNear("left channel RMS", result.channels[0].rms, 0.5)
        && tests::expectNear("right channel RMS", result.channels[1].rms, 0.25)
        && tests::expectNear("all-channel peak", result.combined.peak, 0.5)
        && tests::expectNear("pooled channel RMS", result.combined.rms, std::sqrt(0.15625))
        && tests::expectNear("calibrated whole-file RMS SPL",
                             audio::rmsToDbSpl(result.combined.rms, 1.0), 85.9176003468815);
}

bool silenceHasNegativeInfiniteSpl(const std::filesystem::path& folder)
{
    // Arrange
    const auto path = folder / "silence.wav";
    writeWave(path, { 0.0f, 0.0f });

    // Act
    const auto result = audio::analyseAudioFile(path.string());
    const double spl = audio::rmsToDbSpl(result.combined.rms, 1.0);

    // Assert
    return tests::expectTrue("silence", result.error.empty() && result.combined.peak == 0.0
        && std::isinf(spl) && spl < 0.0);
}

bool rejectsInvalidFilesAndSamples(const std::filesystem::path& folder)
{
    // Arrange
    const auto corrupt = folder / "corrupt.wav";
    std::ofstream(corrupt) << "not audio";
    const auto empty = folder / "empty.wav";
    writeWave(empty, {});
    const auto nonfinite = folder / "nonfinite.wav";
    writeWave(nonfinite, { std::numeric_limits<float>::quiet_NaN() });

    // Act
    const auto missingResult = audio::analyseAudioFile((folder / "missing.wav").string());
    const auto corruptResult = audio::analyseAudioFile(corrupt.string());
    const auto emptyResult = audio::analyseAudioFile(empty.string());
    const auto nonfiniteResult = audio::analyseAudioFile(nonfinite.string());

    // Assert
    return tests::expectTrue("missing file rejected", !missingResult.error.empty())
        && tests::expectTrue("corrupt file rejected", !corruptResult.error.empty())
        && tests::expectTrue("empty audio rejected", !emptyResult.error.empty())
        && tests::expectTrue("non-finite samples rejected", !nonfiniteResult.error.empty());
}

bool analysisCanBeCancelled(const std::filesystem::path& folder)
{
    // Arrange
    const auto path = folder / "cancel.wav";
    writeWave(path, { 0.5f, -0.5f });
    std::stop_source stop;
    stop.request_stop();

    // Act
    const auto result = audio::analyseAudioFile(path.string(), stop.get_token());

    // Assert
    return tests::expectTrue("analysis honours cancellation", result.cancelled);
}
}

int main()
{
    const auto unique = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto folder = std::filesystem::temp_directory_path()
        / ("audio-level-tests-" + std::to_string(unique));
    std::filesystem::create_directory(folder);
    bool passed = false;
    try
    {
        const std::array results {
            measuresMonoAndPreservesOverFullScale(folder),
            weightsTheFinalShortChunkCorrectly(folder),
            retainsPrecisionUntilWholeFileRms(folder),
            measuresStereoWithoutCancellation(folder),
            silenceHasNegativeInfiniteSpl(folder),
            rejectsInvalidFilesAndSamples(folder),
            analysisCanBeCancelled(folder)
        };
        passed = std::all_of(results.begin(), results.end(), [](bool result) { return result; });
    }
    catch (const std::exception& error)
    {
        std::cerr << "Fixture failure: " << error.what() << '\n';
    }
    std::filesystem::remove_all(folder);
    return passed ? 0 : 1;
}
