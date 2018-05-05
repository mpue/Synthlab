/*
  ==============================================================================

    MultiplyModule.h
    Created: 16 Apr 2018 1:48:26pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"

//==============================================================================
/*
 */
class MultiplyModule    : public Module
{
public:
    MultiplyModule();
    ~MultiplyModule();
    
    
    void paint (juce::Graphics& g) override;
    virtual void process() override;
    virtual void configurePins() override;
    
    virtual juce::String getCategory() override {
        return "Math";
    }
    
private:
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MultiplyModule)
};
