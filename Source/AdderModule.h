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
class AdderModule    : public Module
{
public:
    AdderModule();
    ~AdderModule();
   
    
    void paint (Graphics& g) override;
    virtual void process() override;
    virtual void configurePins() override;
    
private:


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdderModule)
};
