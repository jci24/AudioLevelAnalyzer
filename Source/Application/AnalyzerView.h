#pragma once

#include "Metering/LevelMeterComponent.h"
#include "Metering/LiveLevelMeter.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

// IDs match the native reference selector, without exposing its widget to the coordinator.
enum class LevelReference { digital = 1, calibratedSpl = 2, simulatedSpl = 3 };

class AnalyzerView final : public juce::Component
{
public:
    AnalyzerView();
    void paint(juce::Graphics&) override;
    void resized() override;

    std::function<void()> onOpen, onPlay, onPause, onStop, onSettingsChanged;
    LevelReference reference() const;
    juce::String calibrationText() const;
    void showFile(const juce::File& file);
    void setLoading(bool loading);
    void showFileDetails(const juce::String& details, bool error = false);
    void showWholeFileLevels(const juce::String& peak, const juce::String& rms);
    void showReferenceValidity(bool valid);
    void showPlayback(bool loaded, bool playing, bool canStop,
                      const juce::String& position, const juce::String& error);
    void updateMeter(int channel, const audio::LiveChannelLevels& levels,
                     bool available, double elapsedSeconds, double referenceOffsetDb);

private:
    juce::Label fileInfo, status, summaryHeading, peakHeading, rmsHeading, peakValue, rmsValue;
    juce::Label referenceLabel, calibrationLabel, calibrationError;
    juce::Label liveHeading, liveChannelOne, liveChannelTwo, playbackStatus;
    std::array<LevelMeterComponent, 2> liveMeters;
    juce::TextButton loadButton { "Open..." }, playButton { "Play" }, pauseButton { "Pause" }, stopButton { "Stop" };
    juce::TextEditor calibration;
    juce::ComboBox levelReference;
    juce::TooltipWindow tooltips { nullptr, 650 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnalyzerView)
};
