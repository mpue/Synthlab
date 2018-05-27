/*
 ==============================================================================
 
 LedModule.h
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
class LedModule    : public Module
{
public:
    LedModule();
    ~LedModule();
    
    void paint (juce::Graphics& g) override;
    
    virtual void process() override;
    
    virtual void configurePins() override;
    
    virtual juce::String getCategory() override {
        return "Debug";
    }
    
    virtual Layer getLayer() override {
        return Layer::GUI;
    };
    
private:
    float value = 0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LedModule)
};
