# Audio Level Analyzer

A JUCE / C++ learning and portfolio project by Jaime Castresana.

**Status: initial project scaffold.** The app opens a resizable desktop window.
Audio capture, level measurement, meters, and DSP are not implemented yet.

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

## Project structure

- `Source/Main.cpp` — application lifecycle and native window.
- `Source/MainComponent.*` — placeholder application component.
- `CMakeLists.txt` / `CMakePresets.json` — dependencies and build configuration.
- `.vscode/` — build tasks, debugger, and editor recommendations.
- `.github/workflows/build.yml` — macOS build on pushes and pull requests.

The build uses CMake's JUCE integration, following the
[official JUCE CMake documentation](https://github.com/juce-framework/JUCE/blob/9.0.5/docs/CMake%20API.md).
Only the GUI modules are linked at this stage; the app does not request microphone access.
The VS Code launch configuration currently targets macOS. Other platforms have
not been validated.

## License

This project is licensed under AGPL-3.0-only; see [LICENSE](LICENSE).
JUCE is a separately licensed dependency, available under AGPLv3 or a commercial
license. See the [JUCE license](https://github.com/juce-framework/JUCE/blob/9.0.5/LICENSE.md).
