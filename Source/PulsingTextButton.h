/*
  ==============================================================================

    PulsingTextButton.h

    juce::TextButton with a transient blue glow border, driven by a Timer.
    Call startPulse() to play N sine-shaped fade cycles (default 3 × 450 ms);
    the glow is drawn via paintOverChildren as an inset ring so the button's
    own text and background stay intact.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

class PulsingTextButton : public juce::TextButton,
                          private juce::Timer
{
public:
    using juce::TextButton::TextButton;

    void startPulse (int cycles = 3,
                     int cycleMs = 450,
                     juce::Colour colour = juce::Colour::fromRGB (60, 160, 255))
    {
        glowColour    = colour;
        cycleMillis   = juce::jmax (50, cycleMs);
        totalMillis   = cycleMillis * juce::jmax (1, cycles);
        elapsedMillis = 0;
        intensity     = 0.0f;
        startTimerHz (30);
        repaint();
    }

    void paintOverChildren (juce::Graphics& g) override
    {
        if (intensity <= 0.001f)
            return;

        const auto bounds = getLocalBounds().toFloat();

        // Five inset rings with decreasing alpha — cheaper than a real blur,
        // gives a soft "outer glow" feel against the dark panel background.
        for (int i = 0; i < 5; ++i)
        {
            const float inset = (float) i + 0.5f;
            const float a = intensity * juce::jmax (0.0f, 1.0f - (float) i * 0.18f);
            g.setColour (glowColour.withAlpha (juce::jlimit (0.0f, 0.85f, a)));
            g.drawRoundedRectangle (bounds.reduced (inset),
                                    juce::jmax (0.0f, 4.0f - (float) i),
                                    1.4f);
        }
    }

private:
    void timerCallback() override
    {
        constexpr int tickMs = 1000 / 30;
        elapsedMillis += tickMs;

        if (elapsedMillis >= totalMillis)
        {
            stopTimer();
            intensity = 0.0f;
            repaint();
            return;
        }

        const float t = (float) (elapsedMillis % cycleMillis) / (float) cycleMillis;
        intensity = std::sin (t * juce::MathConstants<float>::pi);
        repaint();
    }

    juce::Colour glowColour { juce::Colours::deepskyblue };
    int totalMillis   = 0;
    int elapsedMillis = 0;
    int cycleMillis   = 450;
    float intensity   = 0.0f;

    JUCE_DECLARE_NON_COPYABLE (PulsingTextButton)
};
