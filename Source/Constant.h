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
class Constant    : public Module
{
public:
    Constant();
    ~Constant();

    void paint (Graphics& g) override;

    void setValue(float value);
    float getValue();
    
private:
    
    float value;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Constant)
};
