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
class SawtoothModule : public Module
{
public:
    SawtoothModule();
    ~SawtoothModule();

    void paint (Graphics& g) override;

    void setPitch(int pitch);
    void setAmplitude(float amplitude);
    void setFine(float fine);
    
private:
    
    int pitch;
    float amplitude;
    float fine = 0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SawtoothModule)
};
