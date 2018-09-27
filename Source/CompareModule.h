/*
  ==============================================================================

    CompareModule.h
    Created: 24 Aug 2018 12:53:21pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once


#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"

//==============================================================================
/*
 */
class CompareModule    : public Module, public ComboBox::Listener
{
public:
    CompareModule();
    ~CompareModule();
    
    
    void paint (juce::Graphics& g) override;
    virtual void process() override;
    virtual void configurePins() override;
    
    virtual juce::String getCategory() override {
        return "Math";
    }
    
    virtual void comboBoxChanged (ComboBox* comboBoxThatHasChanged) override;
    
    void setMode(int mode);
    int getMode();
    
private:
    int mode = 1;
    ComboBox* compareModeCombo = nullptr;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CompareModule)
};
