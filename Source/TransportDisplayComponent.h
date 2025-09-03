#pragma once
#include <JuceHeader.h>

struct TransportLookAndFeel
{
    juce::Colour background = juce::Colour(20, 22, 28);
    juce::Colour backgroundGradientStart = juce::Colour(30, 32, 38);
    juce::Colour backgroundGradientEnd = juce::Colour(15, 17, 23);
    juce::Colour border = juce::Colour(60, 65, 75);
    juce::Colour text = juce::Colour(220, 220, 225);
    juce::Colour textSecondary = juce::Colour(160, 165, 175);
    juce::Colour accent = juce::Colour(100, 150, 255);
    juce::Colour tempoPulse = juce::Colour(255, 140, 50);
    juce::Colour metronomeOn = juce::Colour(50, 200, 80);
    juce::Colour metronomeOff = juce::Colour(120, 40, 40);
    juce::Colour loopOn = juce::Colour(255, 180, 50);
    juce::Colour loopOff = juce::Colour(80, 80, 90);
    juce::Colour glassOverlay = juce::Colour::fromRGBA(255, 255, 255, 25);
    float cornerRadius = 12.0f;
    float borderThickness = 1.5f;
    juce::Font mainFont = juce::Font(17.0f, juce::Font::bold);
    juce::Font secondaryFont = juce::Font(15.0f, juce::Font::plain);
};

class TransportDisplayComponent : public juce::Component, public juce::Timer, public juce::ChangeBroadcaster
{
public:
    enum class LayoutMode { Horizontal, Compact };

    TransportDisplayComponent();

    void setTransportState(double samplePos, double sampleRate, double bpm, int numerator, int denominator);
    void setMetronomeState(bool active);
    void setLoopState(bool active);

    std::function<void()> onToggleMetronome;
    std::function<void()> onToggleLoop;

    void setLayoutMode(LayoutMode mode);
    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    void mouseUp(const juce::MouseEvent& e) override;

    bool isMetronomeActive() const { return metronomeActive; }
    bool isLoopActive() const { return loopActive; }

	double getCurrentBpm() const { return currentBpm; }

private:
    double currentSamplePos = 0;
    double currentSampleRate = 44100.0;
    double currentBpm = 120.0;
    int timeSigNumerator = 4;
    int timeSigDenominator = 4;
    bool metronomeActive = false;
    bool loopActive = false;
    LayoutMode layoutMode = LayoutMode::Horizontal;
    int bpmPulse = 0;

    juce::Rectangle<int> metronomeArea;
    juce::Rectangle<int> loopArea;
    TransportLookAndFeel lookAndFeel;

    std::unique_ptr<juce::Label> tempoLabel;
    std::unique_ptr<juce::TextEditor> tempoEditor;

    juce::String getTimeAsString(double samples, double rate) const;
    juce::String getBarBeatString(double samples, double rate, double bpm, int numerator, int denominator) const;
    void drawHorizontalLayout(juce::Graphics& g);
    void drawCompactLayout(juce::Graphics& g);
    void drawMetronomeButton(juce::Graphics& g, const juce::Rectangle<int>& area);
    void drawLoopButton(juce::Graphics& g, const juce::Rectangle<int>& area);
    void drawGradientBackground(juce::Graphics& g, const juce::Rectangle<float>& bounds);
    void drawGlassEffect(juce::Graphics& g, const juce::Rectangle<float>& bounds);
};