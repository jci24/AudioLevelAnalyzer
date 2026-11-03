#include "MainComponent.h"

MainComponent::MainComponent()
{
    setSize(800, 500);
}

void MainComponent::paint(juce::Graphics& graphics)
{
    graphics.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    graphics.setColour(juce::Colours::white);
    graphics.setFont(juce::FontOptions(24.0f));
    graphics.drawFittedText("Audio Level Analyzer", getLocalBounds(),
                            juce::Justification::centred, 1);
}
