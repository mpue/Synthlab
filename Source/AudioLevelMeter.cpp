/*
  ==============================================================================

    AudioLevelMeter.cpp
    Created: 1 Sep 2025 4:16:06pm
    Author:  mpue

  ==============================================================================
*/

#include "AudioLevelMeter.h"

AudioLevelMeter::AudioLevelMeter()
{
    startTimerHz(60); // 60 FPS für flüssige Animation
    setOpaque(true);

    // Toggle Button erstellen
    toggleButton = std::make_unique<juce::ToggleButton>();
    toggleButton->setButtonText("POST");
    toggleButton->setToggleState(false, juce::dontSendNotification);
    toggleButton->onClick = [this]()
        {
            bool isToggled = toggleButton->getToggleState();
            toggleButton->setButtonText(isToggled ? "PRE" : "POST");

            if (onToggleCallback)
                onToggleCallback(isToggled);
        };
    addAndMakeVisible(*toggleButton);
}

AudioLevelMeter::~AudioLevelMeter()
{
    stopTimer();
}

void AudioLevelMeter::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Hintergrund
    g.setColour(backgroundColor);
    g.fillRect(bounds);

    if (numChannels == 0)
        return;

    // Platz für Toggle Button reservieren (oben links)
    auto toggleArea = bounds.removeFromTop(30.0f);
    bounds.removeFromTop(5.0f); // 5px Abstand

    // Bereich für Scale und Meter aufteilen
    auto scaleWidth = 50.0f;
    auto scaleArea = bounds.removeFromLeft(scaleWidth);
    auto meterArea = bounds.reduced(2.0f);

    // Skala zeichnen
    drawScale(g, scaleArea);

    // Channel-Breite berechnen (viel schmaler)
    float channelWidth = juce::jmin(25.0f, meterArea.getWidth() / static_cast<float>(numChannels));
    float meterWidth = channelWidth - 2.0f; // 1px Abstand zwischen Kanälen

    // Jeder Kanal
    for (int ch = 0; ch < numChannels; ++ch)
    {
        if (ch >= channels.size())
            continue;

        auto channelBounds = juce::Rectangle<float>(
            meterArea.getX() + ch * channelWidth,
            meterArea.getY(),
            channelWidth,
            meterArea.getHeight()
        );

        auto meterBounds = channelBounds.reduced(1.0f, 0.0f);
        meterBounds.setWidth(meterWidth);

        const auto& channel = channels[ch];

        // Hintergrund des Meters
        g.setColour(borderColor);
        g.drawRect(meterBounds, 1.0f);

        // Level Bar (nur wenn über Minimum)
        if (channel.smoothedDbLevel > MIN_DB)
        {
            float normalizedLevel = dbToNormalized(channel.smoothedDbLevel);
            float levelHeight = meterBounds.getHeight() * normalizedLevel;
            auto levelRect = meterBounds.removeFromBottom(levelHeight);

            // Farbe basierend auf dB-Wert
            g.setColour(getLedColor(channel.smoothedDbLevel,20));
            g.fillRect(levelRect);
        }

        // Peak Hold Line
        if (channel.peakDbLevel > MIN_DB)
        {
            float normalizedPeak = dbToNormalized(channel.peakDbLevel);
            float peakY = levelToY(normalizedPeak, meterBounds);
            g.setColour(juce::Colours::white);
            g.drawHorizontalLine(static_cast<int>(peakY),
                meterBounds.getX(), meterBounds.getRight());
        }

        // Channel Label (unten)
        g.setColour(juce::Colours::white);
        g.setFont(8.0f);
        auto labelArea = channelBounds.removeFromBottom(15.0f);
        g.drawText(juce::String(ch + 1), labelArea,
            juce::Justification::centred, true);

        // dB Wert anzeigen (nur wenn hörbar)
        if (channel.currentDbLevel > MIN_DB + 10.0f) // nur anzeigen wenn > -50dB
        {
            juce::String dbText = juce::String(channel.currentDbLevel, 0);
            auto dbArea = channelBounds.removeFromBottom(12.0f);
            g.setFont(7.0f);
            g.drawText(dbText, dbArea, juce::Justification::centred, true);
        }
    }
}

void AudioLevelMeter::resized()
{
    // Toggle Button positionieren (oben links)
    if (toggleButton)
    {
        toggleButton->setBounds(5, 5, 60, 25);
    }
}

void AudioLevelMeter::timerCallback()
{
    bool needsRepaint = false;

    for (auto& channel : channels)
    {
        // Smoothing für flüssige Animation
        float targetLevel = channel.currentDbLevel;
        float diff = targetLevel - channel.smoothedDbLevel;

        if (std::abs(diff) > 0.1f) // 0.1 dB Schwelle
        {
            channel.smoothedDbLevel += diff * (1.0f - SMOOTHING_FACTOR);
            needsRepaint = true;
        }

        // Peak hold logic
        if (channel.currentDbLevel > channel.peakDbLevel)
        {
            channel.peakDbLevel = channel.currentDbLevel;
            channel.peakHoldCounter = PEAK_HOLD_TIME;
            needsRepaint = true;
        }
        else if (channel.peakHoldCounter > 0)
        {
            --channel.peakHoldCounter;
        }
        else
        {
            // Peak langsam abfallen lassen (1 dB pro Sekunde)
            channel.peakDbLevel -= 1.0f / 60.0f;
            if (channel.peakDbLevel < MIN_DB)
                channel.peakDbLevel = MIN_DB;
            needsRepaint = true;
        }
    }

    if (needsRepaint)
        repaint();
}

void AudioLevelMeter::processAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    auto* buffer = bufferToFill.buffer;
    auto startSample = bufferToFill.startSample;
    auto numSamples = bufferToFill.numSamples;

    if (buffer == nullptr || numSamples == 0)
        return;

    // Anzahl der Kanäle automatisch anpassen
    int currentNumChannels = buffer->getNumChannels();
    if (currentNumChannels != numChannels)
    {
        setNumChannels(currentNumChannels);
    }

    // Für jeden Kanal den Level berechnen
    for (int ch = 0; ch < currentNumChannels; ++ch)
    {
        if (ch >= channels.size())
            continue;

        // RMS Level berechnen
        float rmsLevel = buffer->getRMSLevel(ch, startSample, numSamples);

        // Direkt in dB umwandeln
        float dbLevel;
        if (rmsLevel <= 0.0f)
        {
            dbLevel = MIN_DB;
        }
        else
        {
            dbLevel = 20.0f * std::log10(rmsLevel);
            dbLevel = juce::jlimit(MIN_DB, MAX_DB, dbLevel);
        }

        setLevelDb(ch, dbLevel);
    }
}

void AudioLevelMeter::setLevelDb(int channel, float dbLevel)
{
    if (channel >= 0 && channel < channels.size())
    {
        channels[channel].currentDbLevel = juce::jlimit(MIN_DB, MAX_DB, dbLevel);
    }
}

void AudioLevelMeter::setNumChannels(int newNumChannels)
{
    if (newNumChannels != numChannels)
    {
        std::function<void(void)> changeLambda =
            [=]() {
            numChannels = newNumChannels;
            channels.clear();
            channels.resize(numChannels);
            repaint();
            };
        juce::MessageManager::callAsync(changeLambda);
    }
}

void AudioLevelMeter::reset()
{
    for (auto& channel : channels)
    {
        channel.currentDbLevel = MIN_DB;
        channel.peakDbLevel = MIN_DB;
        channel.smoothedDbLevel = MIN_DB;
        channel.peakHoldCounter = 0;
    }
    repaint();
}

float AudioLevelMeter::dbToNormalized(float db)
{
    return (db - MIN_DB) / (MAX_DB - MIN_DB);
}

float AudioLevelMeter::normalizedToDb(float normalized)
{
    return MIN_DB + (normalized * (MAX_DB - MIN_DB));
}

float AudioLevelMeter::levelToY(float normalizedLevel, const juce::Rectangle<float>& bounds)
{
    return bounds.getBottom() - (normalizedLevel * bounds.getHeight());
}

void AudioLevelMeter::drawScale(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    g.setColour(juce::Colours::lightgrey);
    g.setFont(8.0f);

    // dB Markierungen
    std::vector<float> dbMarks = { 0, -6, -12, -18, -24, -30, -36, -48, -60 };

    for (float dbMark : dbMarks)
    {
        float normalizedLevel = dbToNormalized(dbMark);
        float y = bounds.getBottom() - (normalizedLevel * bounds.getHeight());

        // Linie
        g.drawHorizontalLine(static_cast<int>(y), bounds.getX(), bounds.getRight() - 15.0f);

        // Label
        juce::String label = (dbMark == 0) ? "0" : juce::String(static_cast<int>(dbMark));
        g.drawText(label, bounds.getRight() - 40.0f, y - 6.0f, 35.0f, 12.0f,
            juce::Justification::centredRight, true);
    }
}

void AudioLevelMeter::drawLedBar(juce::Graphics& g, const juce::Rectangle<float>& bounds, float dbLevel, float peakDb)
{
    const int totalLeds = 20; // Anzahl der LED-Segmente
    const float ledHeight = bounds.getHeight() / totalLeds;
    const float ledSpacing = 1.0f;
    const float actualLedHeight = ledHeight - ledSpacing;

    // Berechne welche LEDs leuchten sollen
    float normalizedLevel = dbToNormalized(dbLevel);
    float normalizedPeak = dbToNormalized(peakDb);

    int activeLeds = static_cast<int>(normalizedLevel * totalLeds);
    int peakLed = static_cast<int>(normalizedPeak * totalLeds);

    // Begrenze auf gültigen Bereich
    activeLeds = juce::jlimit(0, totalLeds, activeLeds);
    peakLed = juce::jlimit(0, totalLeds - 1, peakLed);

    for (int led = 0; led < totalLeds; ++led)
    {
        float y = bounds.getBottom() - ((led + 1) * ledHeight);
        juce::Rectangle<float> ledRect(bounds.getX(), y, bounds.getWidth(), actualLedHeight);

        // LED-Farbe bestimmen
        juce::Colour ledColor;
        bool isLit = false;

        if (led < activeLeds)
        {
            // Normale Level-LEDs
            ledColor = getLedColor(led, totalLeds, false);
            isLit = true;
        }
        else if (led == peakLed && peakDb > dbLevel + 1.0f) // Peak nur anzeigen wenn deutlich über Level
        {
            // Peak-LED
            ledColor = getLedColor(led, totalLeds, true);
            isLit = true;
        }
        else
        {
            // Unbeleuchtete LED (dunkel)
            ledColor = getLedColor(led, totalLeds, false).withAlpha(0.1f);
        }

        // LED mit 3D-Effekt zeichnen
        if (isLit)
        {
            // Leuchtende LED mit Glow-Effekt
            g.setColour(ledColor.brighter(0.3f));
            g.fillRoundedRectangle(ledRect.reduced(0.5f), 1.0f);

            // Innerer Glanz
            auto innerRect = ledRect.reduced(1.5f);
            g.setColour(ledColor.brighter(0.6f));
            g.fillRoundedRectangle(innerRect, 0.5f);

            // Highlight oben
            auto highlightRect = innerRect.removeFromTop(innerRect.getHeight() * 0.3f);
            g.setColour(juce::Colours::white.withAlpha(0.4f));
            g.fillRoundedRectangle(highlightRect, 0.5f);
        }
        else
        {
            // Unbeleuchtete LED
            g.setColour(ledColor);
            g.fillRoundedRectangle(ledRect.reduced(0.5f), 1.0f);

            // Dunkler Rand
            g.setColour(juce::Colours::black.withAlpha(0.5f));
            g.drawRoundedRectangle(ledRect.reduced(0.5f), 1.0f, 0.5f);
        }
    }
}

juce::Colour AudioLevelMeter::getLedColor(int ledIndex, int totalLeds, bool isPeak)
{
    if (isPeak)
    {
        return juce::Colours::white; // Peak immer weiß
    }

    // Farbverteilung von unten nach oben
    float position = static_cast<float>(ledIndex) / static_cast<float>(totalLeds);

    if (position < 0.6f) // Untere 60% - Grün
    {
        return juce::Colour(0xff00ff00);
    }
    else if (position < 0.85f) // Mittlere 25% - Gelb/Orange
    {
        float blend = (position - 0.6f) / 0.25f;
        return juce::Colour(0xff00ff00).interpolatedWith(juce::Colour(0xffff8000), blend);
    }
    else // Obere 15% - Rot
    {
        return juce::Colour(0xffff0000);
    }
}
