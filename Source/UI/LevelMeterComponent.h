#pragma once

#include "MeterBallistics.h"
#include <juce_gui_basics/juce_gui_basics.h>

class LevelMeterComponent final : public juce::Component, public juce::SettableTooltipClient
{
public:
    LevelMeterComponent();
    void setLevels(float peak, float rms, double elapsedSeconds, double referenceOffsetDb,
                   bool rmsReady = true);
    void clear(double referenceOffsetDb = 0.0);
    void paint(juce::Graphics& graphics) override;
    void resized() override;

private:
    metering::MeterBallistics movement;
    juce::Label peakReading, rmsReading;
    double scaleOffsetDb = 0.0;
    bool active = false;
};
