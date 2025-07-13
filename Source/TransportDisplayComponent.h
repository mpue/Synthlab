#pragma once

#include <JuceHeader.h>

class TransportDisplayComponent : public juce::Component, public juce::Timer
{
public:
    enum class LayoutMode { Horizontal, Compact };

    TransportDisplayComponent();

    void setTransportState(double samplePos, double sampleRate, double bpm, int numerator, int denominator);
    void setMetronomeState(bool active);
    std::function<void()> onToggleMetronome;

    void setLayoutMode(LayoutMode mode);

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    void mouseUp(const juce::MouseEvent& e) override;

private:
    double currentSamplePos = 0;
    double currentSampleRate = 44100.0;
    double currentBpm = 120.0;
    int timeSigNumerator = 4;
    int timeSigDenominator = 4;
    bool metronomeActive = false;

    LayoutMode layoutMode = LayoutMode::Horizontal;
    int bpmPulse = 0;

    juce::Rectangle<int> metronomeArea;

    juce::String getTimeAsString(double samples, double rate) const;
    void drawHorizontalLayout(juce::Graphics& g);
    void drawCompactLayout(juce::Graphics& g);
    void drawMetronomeButton(juce::Graphics& g, const juce::Rectangle<int>& area);
};

