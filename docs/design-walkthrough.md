# Understand and explain the project

Use these questions to walk through the implementation in an interview or while
learning. The goal is to explain the trade-offs from the code, not memorise claims.

1. **Why can two signals have the same peak but different RMS?**
   Work through `[1, 0, 0, 0]` and `[1, -1, 1, -1]` in `Analysis/SampleRms.cpp`.
2. **Why is whole-file RMS not the average of chunk RMS values?**
   Explain squared-sample totals, sample-count weighting, and a shorter final chunk.
   Follow `Analysis/AudioFileAnalysis.cpp` and its partial-chunk test.
3. **What happens to a click between UI refreshes?**
   Follow pending peaks, publication acknowledgement, and resets in
   `Playback/MeteredAudioSource.cpp`; then read the transient-capture test.
4. **What work must the custom audio callback avoid?**
   Identify its bounded per-sample calculations and atomic publication. Distinguish
   the meter code's guarantees from JUCE's own transport/buffering behavior.
5. **Why can playback and whole-file peaks differ?**
   The playback tap is after sample-rate conversion. Read the test that compares
   its interval peak against actual rendered samples, including the startup edge.
6. **Can an arbitrary WAV be measured in dB SPL?**
   Explain the missing physical calibration and why the default is digital level.
   See [measurement definitions](measurements.md).
7. **Why keep concrete classes instead of interfaces everywhere?**
   Explain which responsibilities are separate, and name a real future replacement
   that would justify an interface. Small files alone are not the design objective.

The prototype demonstrates file analysis, sample-domain measurements, JUCE playback,
and a custom meter-publication design. It does not establish certification, acoustic
accuracy without calibration, extensive device compatibility, or production hardening.
