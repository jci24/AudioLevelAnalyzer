#pragma once

#include "Files/AudioFileAnalysis.h"
#include "Playback/FilePlayer.h"
#include "UI/LevelMeterComponent.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <thread>

class MainComponent final : public juce::Component, private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;
    void paint(juce::Graphics& graphics) override;
    void resized() override;

private:
    void chooseFile();
    void analyseFile(const juce::File& file);
    void updateResults();
    void updatePlaybackControls();
    void updateLiveLevels();
    void timerCallback() override;

    juce::Label fileInfo, status, summaryHeading, peakHeading, rmsHeading, peakValue, rmsValue;
    juce::Label referenceLabel, calibrationLabel, calibrationError;
    juce::Label liveHeading, liveChannelOne, liveChannelTwo;
    std::array<LevelMeterComponent, 2> liveMeters;
    double lastMeterUpdateSeconds = 0.0;
    juce::TextButton loadButton { "Open..." };
    juce::TextButton playButton { "Play" }, pauseButton { "Pause" }, stopButton { "Stop" };
    juce::Label playbackStatus;
    juce::TextEditor calibration;
    juce::ComboBox levelReference;
    std::unique_ptr<juce::FileChooser> chooser;
    audio::FileAnalysis analysis;
    bool hasResult = false;
    juce::File selectedFile;
    audio::FilePlayer player;
    juce::String playbackError;
    juce::TooltipWindow tooltips { nullptr, 650 };
    std::jthread worker;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
