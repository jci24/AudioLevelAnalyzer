# Measurement definitions

## Peak and RMS

For one channel, sample peak is `max(abs(sample))`. Negative excursions matter as
much as positive ones. RMS is `sqrt(sum(sample * sample) / sampleCount)`.
Squaring prevents opposite signs from cancelling; the square root returns an
amplitude-scale value. Squaring and accumulation use double precision.

`[1, 0, 0, 0]` has peak 1 and RMS 0.5. `[1, -1, 1, -1]` has peak 1 and RMS 1.
A 0.5-amplitude sine measured over complete cycles has peak 0.5 and RMS
`0.5 / sqrt(2)`, about 0.353553. A partial final cycle can change the measured RMS.

The pure functions are in `Source/Analysis/SamplePeak.*` and `SampleRms.*`.
Empty buffers return zero. Their input contract is finite floating-point audio.
File analysis rejects non-finite decoded samples before measurement. Magnitudes
above 1.0 remain measurable; the analysis does not clamp them to digital full scale.

## Whole-file versus playback measurements

File analysis reads 4096 frames per chunk. That is a memory/processing choice,
not a requirement that chunks match waveform cycles. The final chunk is measured
using its actual length, without counting unused buffer space.

Channel sums of squares are accumulated directly. Averaging chunk RMS values
would be wrong when chunk lengths or levels differ. The summary peak is the
largest magnitude across all file channels. Summary RMS combines all channel
sample energies equally; channels are not mixed together first.

The playback tap observes rendered samples after JUCE's sample-rate conversion
and before device output. Consequently, playback peaks can differ from original
file peaks, including transients introduced by resampling. A mono file may appear
on both playback outputs.

Live RMS uses consecutive, non-overlapping 100 ms windows at the device sample
rate. Window state carries across device blocks. The most recently completed
window is published until the next completes; no RMS is shown until the first
window completes. Peak publication retains transients between UI reads.

## Digital levels

Peak and RMS use `20 * log10(magnitude / 1.0)`. A full-scale sine therefore reads
0 dBFS peak and approximately -3.01 dBFS RMS. This is the mathematical RMS
reference, not the AES17 full-scale-sine RMS convention. Silence is negative
infinity, not 0 dBFS. Floating-point values can exceed magnitude 1.0.

## Sound pressure level

SPL requires a factor linking digital sample units to pascals:

```text
RMS pressure (Pa) = RMS sample magnitude × pascals per sample unit
SPL = 20 × log10(RMS pressure / 0.000020 Pa)
```

20 micropascals is the pressure reference in air. Thus 1 Pa RMS is approximately
93.98 dB SPL, and doubling pressure adds approximately 6.02 dB. See
[NI's microphone measurement guide](https://www.ni.com/en/shop/data-acquisition/sensor-fundamentals/measuring-sound-with-microphones.html).

The same logarithmic pressure conversion is applied to sample peak for the peak
SPL display. These values are unweighted. No A/C weighting, reconstructed true
peak, certified time weighting, or perceptually weighted loudness is implemented.

Calibrated SPL requires a factor valid for the recording chain and gain. Selecting
that mode is the user's assertion that the factor is appropriate; the app cannot
verify it. Simulated SPL explicitly uses an assumed factor. A file from the internet
normally does not provide enough information to establish original physical SPL.
Neither mode measures the pressure produced by the user's speakers or headphones.

Calibration must be finite and positive. RMS must be finite and nonnegative.
Invalid conversion inputs throw `std::invalid_argument`; silence returns negative
infinity. The UI validates calibration before requesting conversions.

## Visual behavior

The UI refreshes at 30 Hz. Peak and RMS bars rise immediately and fall at 24 dB/s.
The white marker holds a peak for one second, then falls at 12 dB/s. These display
choices do not alter sample measurements or numeric readings. Movement uses elapsed
time, so it does not depend on an assumed UI frame rate.

The vertical scale spans -60 to +6 dB relative to magnitude 1.0. Calibration shifts
its SPL labels. Numeric readings can show values outside the visual range. Both
bars progress from green below -24 dBFS, through yellow at -12 dBFS, to red at 0 dBFS.
Colours express digital headroom, including in SPL mode; they are not acoustic
exposure thresholds. Pause, stop, end-of-file, and file replacement clear live state.
