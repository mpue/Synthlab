/*
  ==============================================================================

    OscilloscopeModule.h
    Created: 16 Apr 2018 3:14:04pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"
#include "AudioEngine/HighPassFilter.h"

//==============================================================================
/*
 */
class OscilloscopeModule : public Module, public Timer
{
public:
    OscilloscopeModule(double sampleRate, int buffersize);
    ~OscilloscopeModule();
    
    void paint (Graphics& g) override;
    
    void process() override;
    virtual void configurePins() override;
    virtual String getCategory() override {
        return "Debug";
    }
    
    void timerCallback() override {
        repaint();
    }
    
    virtual Layer getLayer() override {
        return Layer::GUI;
    };
    
    virtual String getDescription() override {
        return "A soimple oscilloscope";
    }
    
private:

    int currentSample = 0;
    float buffer[1024];
    float xscale = 1;
    float yscale = 10;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OscilloscopeModule)
};
