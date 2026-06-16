/*
  ==============================================================================

    SevenSegmentModule.h
    Created: 15 Jun 2026
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once


#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"

//==============================================================================
/*
    A retro LED-style numeric display. Renders the integer part of the value
    arriving on its VALUE input pin across a fixed number of 7-segment digits.
 */
class SevenSegmentModule    : public Module, public juce::Timer
{
public:
    SevenSegmentModule();
    ~SevenSegmentModule();

    void paint (juce::Graphics& g) override;

    virtual void process() override;

    virtual void configurePins() override;

    virtual juce::String getCategory() override {
        return "Controls";
    }

    // Like the other display/control widgets (LED, Oscilloscope, Image),
    // live on the GUI layer so it can be surfaced in the Controls-only layer.
    virtual Layer getLayer() override {
        return Layer::GUI;
    }

    void timerCallback() override {
        repaint();
    }

private:
    // Draw a single 7-segment digit inside the given cell.
    // segments bit order: a,b,c,d,e,f,g  -> bit0..bit6
    void drawDigit(juce::Graphics& g, int segments,
                   float x, float y, float w, float h);

    float value = 0;
    int numDigits = 4;
    Pin* valueInPin;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SevenSegmentModule)
};
