#pragma once

#include "AnalyzerView.h"
#include "Analysis/AudioFileAnalysis.h"
#include "Playback/FilePlayer.h"
#include <thread>

// Coordinates the use cases. Widget setup and layout belong to AnalyzerView.
class MainComponent final : public juce::Component, private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;
    void resized() override;

private:
    void chooseFile();
    void analyseFile(const juce::File& file);
    void updateResults();
    void updatePlaybackControls();
    void updateLiveLevels();
    void timerCallback() override;

    AnalyzerView view;
    std::unique_ptr<juce::FileChooser> chooser;
    audio::FileAnalysis analysis;
    bool hasResult = false;
    juce::File selectedFile;
    audio::FilePlayer player;
    juce::String playbackError;
    double lastMeterUpdateSeconds = 0.0;
    std::jthread worker;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
