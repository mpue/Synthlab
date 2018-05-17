/*
  ==============================================================================

    DisplayValueModule.h
    Created: 16 Apr 2018 12:51:10pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once


#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"

//==============================================================================
/*
 */
class DisplayValueModule    : public Module, public juce::Timer
{
public:
    DisplayValueModule();
    ~DisplayValueModule();
    
    void paint (juce::Graphics& g) override;

    virtual void process() override;
    
    virtual void configurePins() override;
    
    virtual juce::String getCategory() override {
        return "Debug";
    }
    
    void timerCallback() override {
        repaint();
    }
    
    virtual void eventReceived(Event* e) override;

    
private:
    float value = 0;
    float velocity = 0;
    int note = 0;
    Pin* eventInPin;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DisplayValueModule)
};
