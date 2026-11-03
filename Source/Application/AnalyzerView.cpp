#include "AnalyzerView.h"

namespace
{
const juce::Colour background { 0xff151c26 };
const juce::Colour foreground { 0xffedf2f7 };
const juce::Colour muted { 0xffb4c1d0 };
const juce::Colour accent { 0xff85d9be };
const juce::Colour errorColour { 0xffff9e94 };
}

AnalyzerView::AnalyzerView()
{
    for (auto* label : { &fileInfo, &status, &summaryHeading, &peakHeading, &rmsHeading,
                        &peakValue, &rmsValue, &referenceLabel, &calibrationLabel,
                        &calibrationError, &liveHeading, &liveChannelOne, &liveChannelTwo,
                        &playbackStatus })
    {
        addAndMakeVisible(*label);
        label->setColour(juce::Label::textColourId, foreground);
        label->setFont(juce::FontOptions(14.0f));
    }
    fileInfo.setText("No file selected", juce::dontSendNotification);
    fileInfo.setFont(juce::FontOptions(19.0f, juce::Font::bold));
    status.setColour(juce::Label::textColourId, muted);
    status.setFont(juce::FontOptions(12.0f));
    summaryHeading.setText("Whole file", juce::dontSendNotification);
    for (auto* label : { &summaryHeading, &liveHeading })
        label->setFont(juce::FontOptions(16.0f, juce::Font::bold));
    peakHeading.setText("Peak", juce::dontSendNotification);
    rmsHeading.setText("RMS", juce::dontSendNotification);
    for (auto* label : { &peakHeading, &rmsHeading })
        label->setColour(juce::Label::textColourId, muted);
    for (auto* label : { &peakValue, &rmsValue })
        label->setFont(juce::FontOptions(24.0f));
    peakHeading.setTooltip("Largest sample magnitude across the complete file and all channels.");
    rmsHeading.setTooltip("Whole-file RMS combines all channel sample energies equally, without mixing channels.");
    for (auto* label : { &liveChannelOne, &liveChannelTwo })
    {
        label->setJustificationType(juce::Justification::centred);
        label->setColour(juce::Label::textColourId, muted);
    }
    liveChannelOne.setText("Output 1", juce::dontSendNotification);
    liveChannelTwo.setText("Output 2", juce::dontSendNotification);
    for (auto& meter : liveMeters)
        addAndMakeVisible(meter);

    for (auto* button : { &loadButton, &playButton, &pauseButton, &stopButton })
        addAndMakeVisible(*button);
    loadButton.setColour(juce::TextButton::textColourOffId, background);
    playButton.setColour(juce::TextButton::buttonColourId, accent);
    playButton.setColour(juce::TextButton::textColourOffId, background);
    loadButton.onClick = [this] { if (onOpen) onOpen(); };
    playButton.onClick = [this] { if (onPlay) onPlay(); };
    pauseButton.onClick = [this] { if (onPause) onPause(); };
    stopButton.onClick = [this] { if (onStop) onStop(); };
    playbackStatus.setJustificationType(juce::Justification::centredRight);
    playbackStatus.setColour(juce::Label::textColourId, muted);

    referenceLabel.setText("Reference", juce::dontSendNotification);
    addAndMakeVisible(levelReference);
    levelReference.addItem("Digital (dBFS)", 1);
    levelReference.addItem("Calibrated SPL", 2);
    levelReference.addItem("Simulated SPL", 3);
    levelReference.setSelectedId(1, juce::dontSendNotification);
    levelReference.setTooltip("Digital RMS uses reference 1.0: a full-scale sine is -3.01 dBFS RMS. SPL requires a recording calibration; simulated SPL uses an assumed factor.");
    levelReference.onChange = [this] { if (onSettingsChanged) onSettingsChanged(); };
    addChildComponent(calibration);
    calibration.setText("1.0");
    calibration.setTitle("Pascals per sample unit");
    calibration.setTooltip("Pascals represented by one sample unit. Use the factor for this recording's microphone chain and gain.");
    calibration.setSelectAllWhenFocused(true);
    calibration.onTextChange = [this] { if (onSettingsChanged) onSettingsChanged(); };
    calibrationLabel.setText("Pa / unit", juce::dontSendNotification);
    calibrationError.setColour(juce::Label::textColourId, errorColour);
    showReferenceValidity(true);
    showPlayback(false, false, false, "-- / --", {});
    setSize(780, 620);
}

LevelReference AnalyzerView::reference() const
{
    return static_cast<LevelReference>(levelReference.getSelectedId());
}

juce::String AnalyzerView::calibrationText() const { return calibration.getText(); }

void AnalyzerView::showFile(const juce::File& file)
{
    fileInfo.setText(file.getFileName(), juce::dontSendNotification);
    fileInfo.setTooltip(file.getFullPathName());
}

void AnalyzerView::setLoading(bool loading)
{
    loadButton.setEnabled(!loading);
    if (loading)
        showFileDetails("Analysing...");
}

void AnalyzerView::showFileDetails(const juce::String& details, bool error)
{
    status.setColour(juce::Label::textColourId, error ? errorColour : muted);
    status.setText(details, juce::dontSendNotification);
}

void AnalyzerView::showWholeFileLevels(const juce::String& peak, const juce::String& rms)
{
    peakValue.setText(peak, juce::dontSendNotification);
    rmsValue.setText(rms, juce::dontSendNotification);
}

void AnalyzerView::showReferenceValidity(bool valid)
{
    const bool useSpl = reference() != LevelReference::digital;
    calibration.setVisible(useSpl);
    calibrationLabel.setVisible(useSpl);
    calibrationError.setVisible(!valid);
    calibrationError.setText(valid ? "" : "Use a positive value", juce::dontSendNotification);
    liveHeading.setText(useSpl ? "Live (dB SPL)" : "Live (dBFS)", juce::dontSendNotification);
    resized();
}

void AnalyzerView::showPlayback(bool loaded, bool playing, bool canStop,
                                const juce::String& position, const juce::String& error)
{
    playButton.setEnabled(loaded && !playing);
    pauseButton.setEnabled(loaded && playing);
    stopButton.setEnabled(loaded && canStop);
    loadButton.setColour(juce::TextButton::buttonColourId, loaded ? juce::Colour(0xffa9bbcd) : accent);
    playbackStatus.setColour(juce::Label::textColourId, error.isNotEmpty() ? errorColour : muted);
    playbackStatus.setTooltip(error);
    playbackStatus.setText(error.isNotEmpty() ? "Output unavailable" : position, juce::dontSendNotification);
}

void AnalyzerView::updateMeter(int channel, const audio::LiveChannelLevels& levels,
                              bool available, double elapsedSeconds, double referenceOffsetDb)
{
    auto& meter = liveMeters[static_cast<std::size_t>(channel)];
    if (available)
        meter.setLevels(levels.peak, levels.rmsReady ? levels.rms : 0.0f,
                        elapsedSeconds, referenceOffsetDb, levels.rmsReady);
    else
        meter.clear(referenceOffsetDb);
}

void AnalyzerView::paint(juce::Graphics& graphics)
{
    graphics.fillAll(background);
    graphics.setColour(juce::Colour(0xff303d4c));
    graphics.drawHorizontalLine(184, 24.0f, static_cast<float>(getWidth() - 24));
    graphics.drawVerticalLine(244, 205.0f, static_cast<float>(getHeight() - 24));
}

void AnalyzerView::resized()
{
    const int width = getWidth();
    fileInfo.setBounds(24, 20, width - 164, 30);
    loadButton.setBounds(width - 124, 20, 100, 30);
    status.setBounds(24, 55, width - 48, 22);
    playButton.setBounds(24, 90, 78, 30);
    pauseButton.setBounds(110, 90, 78, 30);
    stopButton.setBounds(196, 90, 78, 30);
    playbackStatus.setBounds(294, 90, width - 318, 30);
    referenceLabel.setBounds(24, 140, 70, 28);
    levelReference.setBounds(98, 140, 205, 28);
    calibrationLabel.setBounds(323, 140, 65, 28);
    calibration.setBounds(393, 140, 90, 28);
    calibrationError.setBounds(493, 140, width - 517, 28);

    summaryHeading.setBounds(24, 205, 200, 25);
    peakHeading.setBounds(24, 250, 200, 22);
    peakValue.setBounds(24, 278, 200, 36);
    rmsHeading.setBounds(24, 340, 200, 22);
    rmsValue.setBounds(24, 368, 200, 36);
    liveHeading.setBounds(274, 205, width - 298, 25);
    const int channelWidth = (width - 318) / 2;
    liveChannelOne.setBounds(274, 240, channelWidth, 22);
    liveChannelTwo.setBounds(294 + channelWidth, 240, channelWidth, 22);
    liveMeters[0].setBounds(274, 270, channelWidth, getHeight() - 294);
    liveMeters[1].setBounds(294 + channelWidth, 270, channelWidth, getHeight() - 294);
}
