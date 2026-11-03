cmake_minimum_required(VERSION 3.24)

project(AudioLevelAnalyzer VERSION 0.1.0 LANGUAGES C CXX)

include(FetchContent)

# JUCE 9.0.5, pinned to a release commit and verified archive checksum.
FetchContent_Declare(JUCE
    URL https://codeload.github.com/juce-framework/JUCE/tar.gz/d60b7a78030661574a615b586d9325ba7a7fa7dd
    URL_HASH SHA256=8237b2976871982eb9107fd67b576d95bf02355d99540a0127c4e7c001ecd9f5
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_gui_app(AudioLevelAnalyzer
    PRODUCT_NAME "Audio Level Analyzer"
    COMPANY_NAME "Jaime Castresana"
    BUNDLE_ID "com.jaimecastresana.audiolevelanalyzer"
    NEEDS_WEB_BROWSER FALSE
    NEEDS_CURL FALSE)

target_sources(AudioLevelAnalyzer PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp
    Source/MainComponent.h)

target_compile_features(AudioLevelAnalyzer PRIVATE cxx_std_20)
target_compile_definitions(AudioLevelAnalyzer PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:AudioLevelAnalyzer,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:AudioLevelAnalyzer,JUCE_VERSION>")

target_link_libraries(AudioLevelAnalyzer PRIVATE
    juce::juce_gui_basics
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)
