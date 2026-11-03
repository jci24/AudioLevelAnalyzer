# Audio Level Analyzer

## Register
product

## Purpose and users
A native JUCE/C++ macOS learning and portfolio project for inspecting audio files.
The user loads a file, inspects peak/RMS measurements, and monitors levels during playback.

## Design principles
- Let the file, transport controls, and live meters lead; avoid explanatory paragraphs.
- Keep digital, calibrated SPL, and simulated SPL references explicit.
- Show errors when needed; put detailed measurement conventions in tooltips and documentation.
- Use compact whole-file summaries and dominant vertical peak/RMS meters.
- Use green through yellow to red for digital headroom, with a white held-peak marker.
- Preserve numeric labels and scale markings so colour is not the only signal.

## Existing visual system
Dark neutral JUCE window, system sans-serif, mint action accent. Retain this identity.
No decorative imagery or browser UI. The native window already supplies the application title.

## Avoid
Repeated instructional text, the bottom per-channel text box, large metric cards,
and duplicated titles that compete with the meters.
