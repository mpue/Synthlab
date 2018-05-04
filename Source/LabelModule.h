/*
  ==============================================================================

    LabelModule.h
    Created: 16 Apr 2018 7:38:38pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once


#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"

//==============================================================================
/*
 */
class LabelModule    : public Module
{
public:
    LabelModule();
    LabelModule(juce::String name);
    ~LabelModule();
    
    void paint (juce::Graphics& g) override;

    virtual void configurePins() override;
    virtual void setSelected(bool selected) override;
    virtual juce::String getCategory() override {
        return "Controls";
    }
    
    virtual Layer getLayer() override {
        return Layer::GUI;
    };
    
    
    
private:
    
    float value;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LabelModule)
};
