#include "LevelMeterComponent.h"

#include <cmath>
#include <limits>

namespace
{
double magnitudeToDb(float magnitude)
{
    return magnitude > 0.0f ? 20.0 * std::log10(static_cast<double>(magnitude))
                            : -std::numeric_limits<double>::infinity();
}

float scalePosition(double db)
{
    constexpr double low = metering::MeterBallistics::floorDb;
    constexpr double high = metering::MeterBallistics::ceilingDb;
    return static_cast<float>(juce::jlimit(0.0, 1.0, (db - low) / (high - low)));
}

juce::Colour colourAtLevel(double db)
{
    const juce::Colour green { 0xff58cf86 };
    const juce::Colour yellow { 0xffffd35b };
    const juce::Colour red { 0xffff655f };
    if (db <= -24.0)
        return green;
    if (db < -12.0)
        return green.interpolatedWith(yellow, static_cast<float>((db + 24.0) / 12.0));
    return yellow.interpolatedWith(red, static_cast<float>(juce::jlimit(0.0, 1.0, (db + 12.0) / 12.0)));
}

struct MeterLayout
{
    float firstX, secondX, width, top, bottom, scaleX;
};

MeterLayout meterLayout(int width, int height)
{
    const float barWidth = juce::jlimit(22.0f, 40.0f, (static_cast<float>(width) - 100.0f) / 3.0f);
    constexpr float gap = 26.0f;
    const float firstX = (static_cast<float>(width) - (2.0f * barWidth + gap + 50.0f)) / 2.0f;
    const float secondX = firstX + barWidth + gap;
    return { firstX, secondX, barWidth, 24.0f, static_cast<float>(height) - 34.0f,
             secondX + barWidth + 8.0f };
}

juce::String readingText(double db)
{
    return std::isinf(db) ? juce::String("-inf") : juce::String(db, 1);
}
}

LevelMeterComponent::LevelMeterComponent()
{
    for (auto* label : { &peakReading, &rmsReading })
    {
        addAndMakeVisible(*label);
        label->setColour(juce::Label::textColourId, juce::Colour(0xffedf2f7));
        label->setFont(juce::FontOptions(14.0f));
        label->setJustificationType(juce::Justification::centred);
        label->setText("--", juce::dontSendNotification);
    }
    peakReading.setTitle("Live peak");
    rmsReading.setTitle("Live RMS");
    peakReading.setTooltip("Largest sample peak captured since the last display update. The white line holds a peak for one second.");
    rmsReading.setTooltip("RMS measured over 100 ms. Numeric readings are not smoothed by the falling bars.");
    setTooltip("Green to red indicates digital headroom, including in SPL mode. It is not an acoustic exposure scale.");
}

void LevelMeterComponent::setLevels(float peak, float rms, double elapsedSeconds,
                                    double referenceOffsetDb, bool rmsReady)
{
    active = true;
    scaleOffsetDb = referenceOffsetDb;
    movement.update(magnitudeToDb(peak), magnitudeToDb(rms), elapsedSeconds);
    peakReading.setText(readingText(magnitudeToDb(peak) + scaleOffsetDb), juce::dontSendNotification);
    rmsReading.setText(rmsReady ? readingText(magnitudeToDb(rms) + scaleOffsetDb) : "--",
                        juce::dontSendNotification);
    repaint();
}

void LevelMeterComponent::clear(double referenceOffsetDb)
{
    if (!active && std::abs(scaleOffsetDb - referenceOffsetDb) < 1.0e-9)
        return;
    active = false;
    scaleOffsetDb = referenceOffsetDb;
    movement.reset();
    peakReading.setText("--", juce::dontSendNotification);
    rmsReading.setText("--", juce::dontSendNotification);
    repaint();
}

void LevelMeterComponent::resized()
{
    const auto layout = meterLayout(getWidth(), getHeight());
    const int readingWidth = static_cast<int>(layout.width + 26.0f);
    peakReading.setBounds(static_cast<int>(layout.firstX) - 13, getHeight() - 26, readingWidth, 24);
    rmsReading.setBounds(static_cast<int>(layout.secondX) - 13, getHeight() - 26, readingWidth, 24);
}

void LevelMeterComponent::paint(juce::Graphics& graphics)
{
    const auto layout = meterLayout(getWidth(), getHeight());
    const float height = layout.bottom - layout.top;
    const auto levelY = [&](double db) { return layout.bottom - height * scalePosition(db); };
    const juce::Colour muted { 0xffb4c1d0 };
    graphics.setFont(juce::FontOptions(12.0f));

    const auto drawMeter = [&](const juce::String& caption, float x, double displayedDb)
    {
        graphics.setColour(muted);
        graphics.drawText(caption, static_cast<int>(x) - 10, 0, static_cast<int>(layout.width) + 20,
                           18, juce::Justification::centred);
        // Fixed colour zones rise from green at the bottom to red at full scale.
        for (float y = layout.bottom - 4.0f; y >= layout.top; y -= 5.0f)
        {
            const double db = metering::MeterBallistics::floorDb
                + static_cast<double>((layout.bottom - y - 2.0f) / height)
                    * (metering::MeterBallistics::ceilingDb - metering::MeterBallistics::floorDb);
            graphics.setColour(active && y + 2.0f >= levelY(displayedDb)
                ? colourAtLevel(db) : juce::Colour(0xff293646));
            graphics.fillRect(x, y, layout.width, 4.0f);
        }
    };
    drawMeter("Peak", layout.firstX, movement.peakBarDb());
    drawMeter("RMS", layout.secondX, movement.rmsBarDb());

    if (active && movement.heldPeakDb() > metering::MeterBallistics::floorDb)
    {
        graphics.setColour(juce::Colours::white);
        graphics.fillRect(layout.firstX - 3.0f, levelY(movement.heldPeakDb()) - 1.0f,
                           layout.width + 6.0f, 2.0f);
    }
    graphics.setColour(muted);
    graphics.setFont(juce::FontOptions(11.0f));
    for (const double tick : { -60.0, -48.0, -36.0, -24.0, -12.0, 0.0, 6.0 })
    {
        const float y = levelY(tick);
        graphics.drawHorizontalLine(static_cast<int>(y), layout.scaleX - 4.0f, layout.scaleX - 1.0f);
        const double value = tick + scaleOffsetDb;
        const juce::String text = (value > 0.0 ? juce::String("+") : juce::String {})
            + juce::String(value, std::abs(scaleOffsetDb) > 0.0001 ? 1 : 0);
        graphics.drawFittedText(text, static_cast<int>(layout.scaleX + 7.0f), static_cast<int>(y) - 8,
                                39, 16, juce::Justification::centredLeft, 1);
    }
}
