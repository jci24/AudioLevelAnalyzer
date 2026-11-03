# Audio Level Analyzer

A JUCE / C++ learning and portfolio project by Jaime Castresana.

**Status: audio-file analysis prototype.** The JUCE app loads an audio file and
shows a whole-file peak/RMS summary beside live vertical output meters.
Digital levels are the default; SPL modes require an explicit calibration factor.
The detectors also have a standalone learning example.
Loaded files can be played, paused, and stopped. Microphone capture is not implemented.

## Analyse an audio file

1. Launch the app and click **Load audio file...**.
2. Choose a file from JUCE's available formats (including WAV, AIFF, FLAC, Ogg,
   and MP3; additional formats depend on the platform).
3. Leave **Reference** on **Digital (dBFS)** for ordinary files.
4. For SPL, choose **Calibrated SPL** and enter **Pa / unit** for
   the recording chain and gain. The same factor applies to every channel.
   **Simulated SPL** explicitly uses an assumed factor (`1.0` initially).

Digital peak and RMS are both calculated as `20 * log10(magnitude / 1.0)`.
This uses the mathematical RMS reference: a full-scale sine has 0 dBFS peak
and approximately -3.01 dBFS RMS. This is not the AES17 sine-referenced RMS
convention. Calibration controls are only shown in SPL modes. Measurement
details are available in tooltips, with inline messages for errors.

Use **Play** to start or resume, **Pause** to keep the current position, and
**Stop** to return to the beginning. After reaching the end, Play restarts
the file. Loading another file stops and replaces the previous one. Controls
are disabled until a valid file has been analysed and opened for playback.
The position display shows elapsed and total seconds.

Playback uses the default audio output and JUCE's buffered file transport,
with sample-rate conversion when needed. The output device is opened on the
first Play; no microphone channels are requested. Mono/stereo playback is
supported (for files with more channels, only the first two are played).
Playback and calibration do not change the stored whole-file measurements;
the SPL readings describe the file under the supplied calibration, not the
sound pressure produced by the current speaker or volume setting.

The main peak is the largest sample magnitude across every channel and the
entire file. Main RMS combines the energy of all channel samples equally;
channels are not mixed together. The bottom per-channel text box has been
removed to keep the window focused on the summary and live meters.
Both pressure levels are unweighted and referenced to 20 micropascals.
Peak is sample peak, not reconstructed true peak. Silence displays `-inf` in
the selected reference unit.

Analysis reads 4096-frame chunks on a background thread. The final partial
chunk uses its actual sample count, so whole-file RMS is energy-weighted,
not an arithmetic average of chunk RMS readings. Calibration edits update
the displayed results without rereading the file. Empty, corrupt, unreadable,
and non-finite audio produce errors instead of partial results. Closing the
window requests cancellation and joins the worker before destroying the UI.

## Live playback meters

Press Play to see peak and RMS bars and numbers for up to two playback output channels.
The audio tap runs after transport sample-rate conversion and before device
output. Mono may appear on both output channels. These are digital stream
measurements; even in SPL mode they do not measure sound at the listener.

Peak captures the largest magnitude between UI reads, so a spike is retained
even if quieter blocks follow before the next display refresh. The audio
thread clears its pending peak only after the UI acknowledges the publication;
if they overlap, retaining it for another refresh avoids losing a transient.
RMS uses consecutive, non-overlapping
100 ms windows at the device sample rate, carrying squared samples across
block boundaries. The most recently completed RMS window is displayed until
the next completes. The initial RMS reading stays blank until the first window completes.

The UI refreshes at 30 Hz. Pause, stop, end-of-file, and file replacement clear
the live readings; resuming starts a fresh RMS window. Whole-file readings stay
separate.

Each output has separate vertical Peak and RMS bars, with numeric values below
and a shared scale beside them. Bars rise immediately and fall at
24 dB per second. A white peak marker holds for 1 second after the last equal
or higher peak, then falls at 12 dB per second. These are visual choices, not
standardised sound-level-meter time weightings. Actual numeric peak/RMS readings
are not smoothed by this display logic. Timing uses elapsed seconds rather
than assuming a fixed frame rate.

The visual scale covers -60 to +6 dB relative to digital magnitude 1.0. In SPL
mode the scale labels shift by the supplied calibration offset. Values outside
the visual range remain visible in the numeric readings. Both meters use fixed
colour zones: green below -24 dBFS, gradually yellow at -12 dBFS, then red at
0 dBFS and above. Those colours describe digital headroom, not acoustic
exposure thresholds, including when the scale is displayed in SPL.

The added measurement callback allocates no memory, draws no UI, performs no
file access, and adds no mutex. Audio-thread-owned accumulators publish via
lock-free atomics, with publication/acknowledgement counters for transient capture
and no compare-and-swap retry loop. Reset generations hide stale publications while the audio
thread clears its own state. JUCE handles decoding/read-ahead and transport
synchronisation separately.

## Development setup (macOS + Visual Studio Code)

- Xcode and its command-line tools (`xcode-select -p` should point to Xcode).
- CMake 3.24 or newer, Ninja, and a C++20 compiler.
- Visual Studio Code with C/C++, CMake Tools, and CodeLLDB (recommended in this folder).

Install build tools with Homebrew:

```sh
brew install cmake ninja
```

Open this folder in VS Code. Select the **debug** configure preset when prompted.
Press **Cmd+Shift+B** to configure and build; press **F5** to build and debug.
For Release, run **Tasks: Run Task → Build Release**.
If VS Code was open before installing build tools, restart it so it picks up the updated PATH.

Or build from a terminal in the project root:

```sh
cmake --preset debug
cmake --build --preset debug
open "build/debug/AudioLevelAnalyzer_artefacts/Debug/Audio Level Analyzer.app"
```

Release build:

```sh
cmake --preset release
cmake --build --preset release
open "build/release/AudioLevelAnalyzer_artefacts/Release/Audio Level Analyzer.app"
```

The first configure downloads JUCE 9.0.5 automatically. Its source archive is
pinned to a release commit and checked with SHA-256. No Projucer or separate
JUCE installation is required. Build output and downloaded dependencies stay
under `build/` and are excluded from Git.

## Sine-wave learning example

`Examples/SineWaveSamples.cpp` generates a 1 kHz sine wave at
48 kHz, stores it in a 512-sample mono `float` buffer (about 10.67 ms), and
prints its measured sample peak, RMS, and simulated dB SPL. It does not play audio.
This console example is separate from the file-analysis window.

Run from the VS Code terminal in the project root:

```sh
cmake --preset debug
cmake --build build/debug --target SineWaveSamples
./build/debug/SineWaveSamples
```

## Peak detector: first step

`audio::findSamplePeak()` in `Source/Audio/SamplePeak.*` receives one channel
of finite floating-point samples and returns the largest absolute value.
For example, `[0.1, -0.8, 0.3]` returns `0.8`. Silence and empty buffers
return `0`. Values above `1.0` are measured without clipping.

The detector does not modify the samples, allocate memory, or remember a
previous buffer's peak. The sine-wave example uses this same function.
The file analyser reuses this function to track the peak across chunks.
Live meters and true-peak estimation are not implemented.

Build and run the known-input checks:

```sh
cmake --preset debug
cmake --build build/debug --target PeakDetectorTests RmsDetectorTests SoundPressureLevelTests AudioFileAnalysisTests LiveLevelMeterTests PlaybackMeterTests MeterBallisticsTests
ctest --test-dir build/debug --output-on-failure -V
```

The checks cover silence, empty buffers, negative and positive peaks,
first/last positions, a single sample, magnitudes above one, independent
calls, and preservation of the input. They also run in GitHub Actions.

## RMS detector

`audio::findSampleRms()` in `Source/Audio/SampleRms.*` measures the root mean
square of one buffer of finite samples. It squares each sample, averages
the squares, and takes the square root. An empty buffer returns zero.
It reads the samples without changing them or retaining state, and uses
double precision for squaring and accumulation.

Peak asks how large the biggest sample is. RMS describes the signal's
effective level across the whole buffer. For example, `[1, 0, 0, 0]`
has peak `1` and RMS `0.5`, while `[1, -1, 1, -1]` has peak `1` and RMS `1`.
RMS is not a perceptually weighted loudness measurement.

Our 0.5-amplitude sine has peak `0.5` and RMS about `0.352156` over the
512-sample buffer. Over complete cycles its RMS would be `0.5 / sqrt(2)`,
about `0.353553`. The partial final cycle accounts for the difference.

The RMS checks include silence, empty buffers, negative values, sustained
levels versus isolated spikes, a four-point sine cycle, magnitudes above
one, very large/small finite values, independent calls, and unchanged input.

## Sound pressure level (dB SPL)

`audio::rmsToDbSpl(rms, pascalsPerSampleUnit)` converts RMS sample magnitude
to unweighted RMS sound pressure level relative to 20 micropascals in air:

```text
RMS pressure (Pa) = RMS sample magnitude × pascals per sample unit
dB SPL = 20 × log10(RMS pressure / 0.000020 Pa)
```

The calibration argument is required. The learning example explicitly assumes
`1 sample unit = 1 Pa` and labels the result **simulated SPL**. Its sine wave
therefore produces about `84.9141 dB SPL`. This is not a measurement of sound
in the room. Real microphone measurements require a calibrated recording
chain with unchanged gain; ordinary audio files alone do not establish SPL.

Silence returns negative infinity, not 0 dB SPL: 0 dB SPL represents the
reference pressure. Negative RMS, nonpositive calibration, and nonfinite
inputs throw `std::invalid_argument`. This step does not add frequency/time
weighting or a calibrated sound-level-meter implementation. The file UI also
applies the same logarithmic pressure conversion to the sample peak magnitude.

AAA checks cover 20 micropascals → 0 dB SPL, 1 Pa → about 93.98 dB SPL,
calibration scaling, negative dB levels below the reference, doubling pressure
→ about +6.02 dB, silence, and invalid inputs. Reference:
[NI's microphone measurement guide](https://www.ni.com/en/shop/data-acquisition/sensor-fundamentals/measuring-sound-with-microphones.html).

## Project structure

Detector tests use Arrange–Act–Assert: each named test prepares samples and
an expected value, calls the detector, and checks the result. Shared reporting
and floating-point comparisons live in `Tests/TestAssertions.h`.

- `Source/Main.cpp` — application lifecycle and native window.
- `Source/MainComponent.*` — file chooser, calibration controls, background job, and results.
- `Source/Files/AudioFileAnalysis.*` — chunked whole-file and per-channel analysis.
- `Source/Playback/FilePlayer.*` — playback transport, output device, pause, and stop.
- `Source/Playback/MeteredAudioSource.*` — rendered-audio tap and atomic publication to the UI.
- `Source/Audio/LiveLevelMeter.*` — block peak and sample-counted 100 ms RMS windows.
- `Source/UI/MeterBallistics.*` — UI-only release and peak-hold timing.
- `Source/UI/LevelMeterComponent.*` — per-channel bars, hold marker, and calibrated scale.
- `Source/Audio/SamplePeak.*` — reusable sample-peak measurement.
- `Source/Audio/SampleRms.*` — reusable RMS measurement.
- `Source/Audio/SoundPressureLevel.*` — RMS-to-dB-SPL conversion with explicit calibration.
- `Tests/PeakDetectorTests.cpp` — known-input detector checks.
- `Tests/RmsDetectorTests.cpp` — known-input RMS checks.
- `Tests/SoundPressureLevelTests.cpp` — SPL conversion and calibration checks.
- `Tests/AudioFileAnalysisTests.cpp` — mono/stereo WAV, partial chunks, silence,
  over-full-scale values, invalid files/samples, and cancellation checks.
- `Tests/LiveLevelMeterTests.cpp` — known signals, window timing, block-size independence, and resets.
- `Tests/PlaybackMeterTests.cpp` — actual resampling/output tap, channel separation, reset publication,
  transient capture between UI reads, end-of-file, and restart, rendered offline without an audio device.
- `Tests/MeterBallisticsTests.cpp` — attack/release, hold duration, frame-rate independence,
  visual limits, and reset behavior.
- `CMakeLists.txt` / `CMakePresets.json` — dependencies and build configuration.
- `.vscode/` — build tasks, debugger, and editor recommendations.
- `.github/workflows/build.yml` — macOS build on pushes and pull requests.

The build uses CMake's JUCE integration, following the
[official JUCE CMake documentation](https://github.com/juce-framework/JUCE/blob/9.0.5/docs/CMake%20API.md).
The app links GUI, audio-file decoding, and output-device modules; it does not request microphone access.
The VS Code launch configuration currently targets macOS. Other platforms have
not been validated.

## License

This project is licensed under AGPL-3.0-only; see [LICENSE](LICENSE).
JUCE is a separately licensed dependency, available under AGPLv3 or a commercial
license. See the [JUCE license](https://github.com/juce-framework/JUCE/blob/9.0.5/LICENSE.md).
