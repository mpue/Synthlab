/*
 ==============================================================================
 
 MidiGate.h
 Created: 4 Apr 2018 3:48:23pm
 Author:  Matthias Pueski
 
 ==============================================================================
 */

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"

//==============================================================================
/*
 */
class PitchBendModule    : public Module
{
public:
    PitchBendModule();
    ~PitchBendModule();
    
    void paint (juce::Graphics& g) override;
    
    void setPitch(int value);
    
    virtual void configurePins() override;
    
    
    virtual juce::String getCategory() override {
        return "MIDI";
    }
    
    
private:
    int pitch = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PitchBendModule)
};

