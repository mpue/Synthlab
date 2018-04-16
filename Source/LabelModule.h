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
    LabelModule(String name);
    ~LabelModule();
    
    void paint (Graphics& g) override;

    virtual void configurePins() override;
    virtual void setSelected(bool selected) override;
    virtual String getCategory() override {
        return "Controls";
    }
    
    virtual Layer getLayer() override {
        return Layer::GUI;
    };
    
    
    
private:
    
    float value;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LabelModule)
};
