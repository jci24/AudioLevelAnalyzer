# Reproduce the portfolio preview

The README image and animation are **offline-rendered UI previews**, not a screen
recording. They use the real `AnalyzerView`, WAV decoder, whole-file analysis,
JUCE transport/resampler, `MeteredAudioSource`, and visual ballistics. No output
device is opened and no personal audio or desktop content is captured.

The renderer feeds ten 10 ms audio blocks into the transport for each 100 ms
animation frame. It renders the same view component used by the app into PNGs.
The animation demonstrates signal response; it does not simulate mouse actions
or establish interactive/device reliability. Native file loading and playback
controls were checked separately in the running app.

On macOS, with the normal project build tools and FFmpeg available:

```sh
python3 Tools/generate_demo.py
cmake --preset debug -DANALYZER_BUILD_DEMO_RENDERER=ON
cmake --build build/debug --target RenderDemo --parallel 4
./build/debug/RenderDemo "$PWD/build/demo/Demo.wav" "$PWD/build/captures/frames"
cp build/captures/frames/analyzer.png docs/media/analyzer.png
ffmpeg -y -framerate 10 -start_number 1000 \
  -i build/captures/frames/frame-%d.png \
  -filter_complex '[0:v]split[a][b];[a]palettegen=max_colors=96[p];[b][p]paletteuse=dither=bayer:bayer_scale=4' \
  -loop 0 docs/media/demo.gif
cmake --preset debug -DANALYZER_BUILD_DEMO_RENDERER=OFF
```

`RenderDemo` is optional and disabled by default. Generated WAVs, intermediate
frames, and builds stay under ignored `build/`; only the final PNG/GIF are committed.
