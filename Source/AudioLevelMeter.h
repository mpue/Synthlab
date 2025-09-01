/*
  ==============================================================================

    AudioLevelMeter.h
    Created: 1 Sep 2025 4:16:06pm
    Author:  mpue

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class AudioLevelMeter : public juce::Component,
    public juce::Timer
{
public:
    AudioLevelMeter();
    ~AudioLevelMeter() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    // Methoden zum Setzen der Pegel
    void setLevel(int channel, float level);
    void setNumChannels(int numChannels);
    void reset();

    // Neue Methode für AudioSourceChannelInfo
    void processAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill);

private:
    struct ChannelMeter
    {
        float currentLevel = 0.0f;
        float peakLevel = 0.0f;
        int peakHoldCounter = 0;
        float smoothedLevel = 0.0f;
    };

    std::vector<ChannelMeter> channels;
    int numChannels = 0;

    // Konstanten
    static constexpr float SMOOTHING_FACTOR = 0.8f;
    static constexpr int PEAK_HOLD_TIME = 30; // Frames
    static constexpr float MIN_DB = -60.0f;
    static constexpr float MAX_DB = 6.0f;

    // Farben
    juce::Colour greenZone = juce::Colours::green;
    juce::Colour yellowZone = juce::Colours::yellow;
    juce::Colour redZone = juce::Colours::red;
    juce::Colour backgroundColor = juce::Colour(0xff2a2a2a);
    juce::Colour borderColor = juce::Colour(0xff404040);

    // Hilfsfunktionen
    float dbToLinear(float db);
    float linearToDb(float linear);
    float levelToY(float level, const juce::Rectangle<float>& bounds);
    void drawScale(juce::Graphics& g, const juce::Rectangle<float>& bounds);
    juce::Colour getLevelColor(float db);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioLevelMeter)
};