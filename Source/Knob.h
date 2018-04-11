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
class Knob    : public Module, Slider::Listener
{
public:
    Knob();
    ~Knob();

    // void paint (Graphics& g) override;

    void setValue(float value);
    float getValue();
    virtual void configurePins() override;
    void sliderValueChanged (Slider* slider) override;
    virtual String getCategory() override {
        return "Controls";
    }
    
    void setStepSize(float stepsize);
    void setMinimum(float min);
    void setMaximum(float max);
    
    float getStepsize();
    float getMinimum();
    float getMaximum();
    
private:
    
    float value;
    Slider* slider;
    
    float minimum = 0 ;
    float maximum = 127;
    float stepsize = 1;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Knob)
};
