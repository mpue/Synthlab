/*
  ==============================================================================

    AudioInSelector.h
    Created: 2026
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"

//==============================================================================
/*
    Routes one of three audio inputs to the output based on a value input.

    Pins:
      0 - VALUE IN  (selector: 0 → A, 1 → B, 2 → C)
      1 - AUDIO IN  A
      2 - AUDIO IN  B
      3 - AUDIO IN  C
      4 - AUDIO OUT
*/
class AudioInSelector : public Module
{
public:
    AudioInSelector(int bufferSize);
    ~AudioInSelector();

    void paint(juce::Graphics& g) override;
    virtual void process() override;
    virtual void configurePins() override;

    virtual juce::String getCategory() override {
        return "Audio";
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioInSelector)
};
