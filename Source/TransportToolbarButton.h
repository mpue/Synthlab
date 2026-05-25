#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

// juce::ToolbarButton variant for Play / Record:
//   - Mode::Solid     — fills the button background with activeColour (Play = green).
//   - Mode::Blinking  — pulses the background at the current BPM (Record = red).
// The icon is drawn on top via the base class' paintButton.
class TransportToolbarButton : public juce::ToolbarButton,
                               private juce::Timer
{
public:
    enum class Mode { Idle, Solid, Blinking };

    using juce::ToolbarButton::ToolbarButton;

    void setActiveColour (juce::Colour c)
    {
        activeColour = c;
    }

    void setActiveMode (Mode m)
    {
        if (mode == m) return;
        mode = m;
        if (mode == Mode::Blinking) startTimerHz (30);
        else                        stopTimer();
        repaint();
    }

    // Drives the blink rate. Quarter-note pulse: one cycle == 60 / bpm seconds.
    void setBlinkBpm (double bpmHint)
    {
        if (bpmHint > 1.0) bpm = bpmHint;
    }

    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        if (mode != Mode::Idle)
        {
            float alpha = 0.85f;
            if (mode == Mode::Blinking)
            {
                // Hard on/off — visually reads as "blinking" rather than a soft glow.
                const double periodMs = 60000.0 / juce::jmax (20.0, bpm);
                const double now      = juce::Time::getMillisecondCounterHiRes();
                const double phase    = std::fmod (now, periodMs) / periodMs; // 0..1
                alpha = (phase < 0.5) ? 0.9f : 0.15f;
            }

            g.setColour (activeColour.withAlpha (alpha));
            g.fillRoundedRectangle (getLocalBounds().reduced (2).toFloat(), 4.0f);
        }

        juce::ToolbarButton::paintButton (g, over, down);
    }

private:
    void timerCallback() override { repaint(); }

    Mode mode { Mode::Idle };
    juce::Colour activeColour { juce::Colours::limegreen };
    double bpm { 120.0 };
};
