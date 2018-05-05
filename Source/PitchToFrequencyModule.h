/*
  ==============================================================================

    PitchToFrequencyModule.h
    Created: 16 Apr 2018 5:55:24pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"

class PitchToFrequencyModule    : public Module
{
public:
    PitchToFrequencyModule();
    ~PitchToFrequencyModule();
    
    void paint (juce::Graphics& g) override;
    
    virtual void configurePins() override;
    virtual void process() override;
    virtual juce::String getCategory() override {
        return "Math";
    }
    
private:
    
    float value;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PitchToFrequencyModule)
};
