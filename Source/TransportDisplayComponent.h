#pragma once

#include <JuceHeader.h>

struct TransportLookAndFeel
{
    juce::Colour background = juce::Colour(25, 25, 30);
    juce::Colour border = juce::Colours::darkgrey.withAlpha(0.8f);
    juce::Colour text = juce::Colours::white;
    juce::Colour tempoPulse = juce::Colours::orange;
    juce::Colour metronomeOn = juce::Colours::lime;
    juce::Colour metronomeOff = juce::Colours::darkred;

    float cornerRadius = 8.0f;
    float borderThickness = 2.0f;
    juce::Font mainFont = juce::Font(18.0f, juce::Font::bold);
};

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

	bool isMetronomeActive() const { return metronomeActive; }  

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
	TransportLookAndFeel lookAndFeel;   

    juce::String getTimeAsString(double samples, double rate) const;
    void drawHorizontalLayout(juce::Graphics& g);
    void drawCompactLayout(juce::Graphics& g);
    void drawMetronomeButton(juce::Graphics& g, const juce::Rectangle<int>& area);

    std::unique_ptr<Label> tempoLabel;
    std::unique_ptr<TextEditor> tempoEditor;

};

