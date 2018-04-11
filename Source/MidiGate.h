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
class MidiGate    : public Module
{
public:
    MidiGate();
    ~MidiGate();

    void paint (Graphics& g) override;

    void gateOn(int velocity);
    void gateOff();
    
    virtual void configurePins() override;
    
    virtual String getCategory() override {
        return "MIDI";
    }
    
    
private:
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiGate)
};
