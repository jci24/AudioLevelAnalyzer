cmake_minimum_required(VERSION 3.24)

project(AudioLevelAnalyzer VERSION 0.1.0 LANGUAGES C CXX)

add_library(AudioAnalysis
    Source/Metering/LiveLevelMeter.cpp
    Source/Analysis/SamplePeak.cpp
    Source/Analysis/SampleRms.cpp
    Source/Analysis/SoundPressureLevel.cpp)
target_include_directories(AudioAnalysis PUBLIC Source)
target_compile_features(AudioAnalysis PUBLIC cxx_std_20)

add_library(MeterDisplay Source/Metering/MeterBallistics.cpp)
target_include_directories(MeterDisplay PUBLIC Source)
target_compile_features(MeterDisplay PUBLIC cxx_std_20)

# A standalone learning example; it does not open the app or an audio device.
add_executable(SineWaveSamples Examples/SineWaveSamples.cpp)
target_link_libraries(SineWaveSamples PRIVATE AudioAnalysis)

include(CTest)
if(BUILD_TESTING)
    add_executable(PeakDetectorTests Tests/Analysis/PeakDetectorTests.cpp)
    target_link_libraries(PeakDetectorTests PRIVATE AudioAnalysis)
    target_include_directories(PeakDetectorTests PRIVATE Tests)
    add_test(NAME PeakDetectorTests COMMAND PeakDetectorTests)

    add_executable(RmsDetectorTests Tests/Analysis/RmsDetectorTests.cpp)
    target_link_libraries(RmsDetectorTests PRIVATE AudioAnalysis)
    target_include_directories(RmsDetectorTests PRIVATE Tests)
    add_test(NAME RmsDetectorTests COMMAND RmsDetectorTests)

    add_executable(SoundPressureLevelTests Tests/Analysis/SoundPressureLevelTests.cpp)
    target_link_libraries(SoundPressureLevelTests PRIVATE AudioAnalysis)
    target_include_directories(SoundPressureLevelTests PRIVATE Tests)
    add_test(NAME SoundPressureLevelTests COMMAND SoundPressureLevelTests)

    add_executable(LiveLevelMeterTests Tests/Metering/LiveLevelMeterTests.cpp)
    target_link_libraries(LiveLevelMeterTests PRIVATE AudioAnalysis)
    target_include_directories(LiveLevelMeterTests PRIVATE Tests)
    add_test(NAME LiveLevelMeterTests COMMAND LiveLevelMeterTests)

    add_executable(MeterBallisticsTests Tests/Metering/MeterBallisticsTests.cpp)
    target_link_libraries(MeterBallisticsTests PRIVATE MeterDisplay)
    target_include_directories(MeterBallisticsTests PRIVATE Tests)
    add_test(NAME MeterBallisticsTests COMMAND MeterBallisticsTests)
endif()

include(FetchContent)

# JUCE 9.0.5, pinned to a release commit and verified archive checksum.
FetchContent_Declare(JUCE
    URL https://codeload.github.com/juce-framework/JUCE/tar.gz/d60b7a78030661574a615b586d9325ba7a7fa7dd
    URL_HASH SHA256=8237b2976871982eb9107fd67b576d95bf02355d99540a0127c4e7c001ecd9f5
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(JUCE)

add_library(AudioFileAnalysis Source/Analysis/AudioFileAnalysis.cpp)
target_link_libraries(AudioFileAnalysis
    PUBLIC AudioAnalysis
    PRIVATE juce::juce_audio_formats juce::juce_recommended_config_flags)
target_compile_definitions(AudioFileAnalysis PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)

if(BUILD_TESTING)
    add_executable(AudioFileAnalysisTests Tests/Analysis/AudioFileAnalysisTests.cpp)
    target_link_libraries(AudioFileAnalysisTests PRIVATE AudioFileAnalysis)
    target_include_directories(AudioFileAnalysisTests PRIVATE Tests)
    add_test(NAME AudioFileAnalysisTests COMMAND AudioFileAnalysisTests)

    add_executable(PlaybackMeterTests
        Tests/Playback/PlaybackMeterTests.cpp
        Source/Playback/MeteredAudioSource.cpp)
    target_link_libraries(PlaybackMeterTests PRIVATE
        AudioAnalysis juce::juce_audio_devices juce::juce_recommended_config_flags)
    target_compile_definitions(PlaybackMeterTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_include_directories(PlaybackMeterTests PRIVATE Tests)
    add_test(NAME PlaybackMeterTests COMMAND PlaybackMeterTests)
endif()

juce_add_gui_app(AudioLevelAnalyzer
    PRODUCT_NAME "Audio Level Analyzer"
    COMPANY_NAME "Jaime Castresana"
    BUNDLE_ID "com.jaimecastresana.audiolevelanalyzer"
    NEEDS_WEB_BROWSER FALSE
    NEEDS_CURL FALSE)

target_sources(AudioLevelAnalyzer PRIVATE
    Source/Application/Main.cpp
    Source/Playback/FilePlayer.cpp
    Source/Playback/MeteredAudioSource.cpp
    Source/Metering/LevelMeterComponent.cpp
    Source/Application/AnalyzerView.cpp
    Source/Application/MainComponent.cpp
    Source/Application/MainComponent.h)

target_compile_features(AudioLevelAnalyzer PRIVATE cxx_std_20)
target_compile_definitions(AudioLevelAnalyzer PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:AudioLevelAnalyzer,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:AudioLevelAnalyzer,JUCE_VERSION>")

target_link_libraries(AudioLevelAnalyzer PRIVATE
    AudioFileAnalysis
    MeterDisplay
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_gui_basics
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)

option(ANALYZER_BUILD_DEMO_RENDERER "Build the offline portfolio media renderer" OFF)
if(ANALYZER_BUILD_DEMO_RENDERER)
    add_executable(RenderDemo
        Tools/RenderDemo.cpp
        Source/Application/AnalyzerView.cpp
        Source/Metering/LevelMeterComponent.cpp
        Source/Playback/MeteredAudioSource.cpp)
    target_link_libraries(RenderDemo PRIVATE
        AudioFileAnalysis MeterDisplay juce::juce_gui_basics
        juce::juce_audio_devices juce::juce_audio_formats juce::juce_recommended_config_flags)
    target_compile_definitions(RenderDemo PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
endif()
