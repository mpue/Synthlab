/*
  ==============================================================================

    InverterModule.h
    Created: 16 Apr 2018 5:09:40pm
    Author:  Matthias Pueski

  ==============================================================================
*/
#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"
class InverterModule    : public Module
{
public:
    InverterModule();
    ~InverterModule();
    
    
    void paint (juce::Graphics& g) override;
    virtual void process() override;
    virtual void configurePins() override;
    
    virtual juce::String getCategory() override {
        return "Math";
    }
    
private:
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InverterModule)
};
