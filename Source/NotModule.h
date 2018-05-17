/*
 ==============================================================================
 
 NotModule.h
 Created: 16 Apr 2018 12:42:07pm
 Author:  Matthias Pueski
 
 ==============================================================================
 */

#pragma once


#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"

//==============================================================================
/*
 */
class NotModule    : public Module
{
public:
    NotModule();
    ~NotModule();
    
    
    void paint (juce::Graphics& g) override;
    virtual void process() override;
    virtual void configurePins() override;
    
    virtual juce::String getCategory() override {
        return "Logic";
    }
    
private:
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NotModule)
};
