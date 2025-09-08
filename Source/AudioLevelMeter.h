/*
  ==============================================================================

    AudioLevelMeter.h
    Created: 1 Sep 2025 4:16:06pm
    Author:  mpue

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <functional>

class AudioLevelMeter : public juce::Component, private juce::Timer
{
public:
    AudioLevelMeter();
    ~AudioLevelMeter() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    // Audio processing
    void processAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill);
    void setLevelDb(int channel, float dbLevel);
    void setNumChannels(int newNumChannels);
    void reset();

    // Toggle button callback
    std::function<void(bool)> onToggleCallback;

private:
    struct ChannelData
    {
        float currentDbLevel = -60.0f;     // aktueller dB-Wert
        float peakDbLevel = -60.0f;        // Peak dB-Wert
        float smoothedDbLevel = -60.0f;    // geglätteter dB-Wert
        int peakHoldCounter = 0;
    };

    std::vector<ChannelData> channels;
    int numChannels = 0;

    // Toggle Button
    std::unique_ptr<juce::ToggleButton> toggleButton;

    // Constants
    static constexpr float MIN_DB = -60.0f;
    static constexpr float MAX_DB = 6.0f;
    static constexpr float SMOOTHING_FACTOR = 0.85f;
    static constexpr int PEAK_HOLD_TIME = 90; // 1.5 Sekunden bei 60 FPS

    // Colors
    juce::Colour backgroundColor = juce::Colour(0xff2a2a2a);
    juce::Colour borderColor = juce::Colour(0xff555555);
    juce::Colour greenZone = juce::Colour(0xff00ff00);
    juce::Colour yellowZone = juce::Colour(0xffffff00);
    juce::Colour redZone = juce::Colour(0xffff0000);

    // Helper functions
    float dbToNormalized(float db);
    float normalizedToDb(float normalized);
    float levelToY(float normalizedLevel, const juce::Rectangle<float>& bounds);
    void drawScale(juce::Graphics& g, const juce::Rectangle<float>& bounds);
    void drawLedBar(juce::Graphics& g, const juce::Rectangle<float>& bounds, float dbLevel, float peakDb);
    juce::Colour getLedColor(int ledIndex, int totalLeds, bool isPeak = false);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioLevelMeter)
};