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

    // Bereich für Scale und Meter aufteilen
    auto scaleWidth = 40.0f;
    auto scaleArea = bounds.removeFromLeft(scaleWidth);
    auto meterArea = bounds.reduced(2.0f);

    // Skala zeichnen
    drawScale(g, scaleArea);

    // Channel-Breite berechnen
    float channelWidth = meterArea.getWidth() / static_cast<float>(numChannels);
    float meterWidth = channelWidth - 4.0f; // 2px Abstand zwischen Kanälen

    // Jeder Kanal
    for (int ch = 0; ch < numChannels; ++ch)
    {
        if (ch >= channels.size())
            continue;

        auto channelBounds = meterArea.removeFromLeft(channelWidth);
        auto meterBounds = channelBounds.reduced(2.0f, 0.0f);
        meterBounds.setWidth(meterWidth);

        const auto& channel = channels[ch];

        // Hintergrund des Meters
        g.setColour(borderColor);
        g.drawRect(meterBounds, 1.0f);

        // Level Bar
        if (channel.smoothedLevel > 0.0f)
        {
            float levelHeight = meterBounds.getHeight() * channel.smoothedLevel;
            auto levelRect = meterBounds.removeFromBottom(levelHeight);

            // Gradient für verschiedene Zonen
            juce::ColourGradient gradient(greenZone, levelRect.getBottomLeft(),
                redZone, levelRect.getTopLeft(), false);
            gradient.addColour(0.7, yellowZone);

            g.setGradientFill(gradient);
            g.fillRect(levelRect);
        }

        // Peak Hold Line
        if (channel.peakLevel > 0.0f)
        {
            float peakY = levelToY(channel.peakLevel, meterBounds);
            g.setColour(juce::Colours::white);
            g.drawHorizontalLine(static_cast<int>(peakY),
                meterBounds.getX(), meterBounds.getRight());
        }

        // Channel Label
        g.setColour(juce::Colours::white);
        g.setFont(10.0f);
        auto labelArea = channelBounds.removeFromBottom(20.0f);
        g.drawText(juce::String(ch + 1), labelArea,
            juce::Justification::centred, true);

        // dB Wert anzeigen
        if (channel.currentLevel > 0.0f)
        {
            float dbValue = linearToDb(channel.currentLevel);
            juce::String dbText = juce::String(dbValue, 1) + "dB";
            auto dbArea = channelBounds.removeFromBottom(15.0f);
            g.setFont(8.0f);
            g.drawText(dbText, dbArea, juce::Justification::centred, true);
        }
    }
}

void AudioLevelMeter::resized()
{
    // Komponente wurde in der Größe geändert
}

void AudioLevelMeter::timerCallback()
{
    bool needsRepaint = false;

    for (auto& channel : channels)
    {
        // Smoothing für flüssige Animation
        float targetLevel = channel.currentLevel;
        float diff = targetLevel - channel.smoothedLevel;

        if (std::abs(diff) > 0.001f)
        {
            channel.smoothedLevel += diff * (1.0f - SMOOTHING_FACTOR);
            needsRepaint = true;
        }

        // Peak hold logic
        if (channel.currentLevel > channel.peakLevel)
        {
            channel.peakLevel = channel.currentLevel;
            channel.peakHoldCounter = PEAK_HOLD_TIME;
            needsRepaint = true;
        }
        else if (channel.peakHoldCounter > 0)
        {
            --channel.peakHoldCounter;
        }
        else
        {
            // Peak langsam abfallen lassen
            channel.peakLevel *= 0.98f;
            if (channel.peakLevel < 0.001f)
                channel.peakLevel = 0.0f;
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

        // Alternative: Peak Level verwenden
        // float peakLevel = buffer->getMagnitude(ch, startSample, numSamples);

        // Level setzen
        setLevel(ch, rmsLevel);
    }
}

void AudioLevelMeter::setLevel(int channel, float level)
{
    if (channel >= 0 && channel < channels.size())
    {
        // Level auf 0.0 - 1.0 normalisieren
        channels[channel].currentLevel = juce::jlimit(0.0f, 1.0f, level);
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
        channel.currentLevel = 0.0f;
        channel.peakLevel = 0.0f;
        channel.smoothedLevel = 0.0f;
        channel.peakHoldCounter = 0;
    }
    repaint();
}

float AudioLevelMeter::dbToLinear(float db)
{
    return std::pow(10.0f, db / 20.0f);
}

float AudioLevelMeter::linearToDb(float linear)
{
    if (linear <= 0.0f)
        return MIN_DB;

    float db = 20.0f * std::log10(linear);
    return juce::jlimit(MIN_DB, MAX_DB, db);
}

float AudioLevelMeter::levelToY(float level, const juce::Rectangle<float>& bounds)
{
    return bounds.getBottom() - (level * bounds.getHeight());
}

void AudioLevelMeter::drawScale(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    g.setColour(juce::Colours::lightgrey);
    g.setFont(9.0f);

    // dB Markierungen
    std::vector<float> dbMarks = { 0, -6, -12, -18, -24, -30, -36, -48, -60 };

    for (float dbMark : dbMarks)
    {
        float normalizedLevel = (dbMark - MIN_DB) / (MAX_DB - MIN_DB);
        float y = bounds.getBottom() - (normalizedLevel * bounds.getHeight());

        // Linie
        g.drawHorizontalLine(static_cast<int>(y), bounds.getX(), bounds.getRight() - 10.0f);

        // Label
        juce::String label = (dbMark == 0) ? "0" : juce::String(static_cast<int>(dbMark));
        g.drawText(label, bounds.getRight() - 30.0f, y - 6.0f, 25.0f, 12.0f,
            juce::Justification::centredRight, true);
    }
}

juce::Colour AudioLevelMeter::getLevelColor(float db)
{
    if (db > -6.0f)
        return redZone;
    else if (db > -18.0f)
        return yellowZone;
    else
        return greenZone;
}