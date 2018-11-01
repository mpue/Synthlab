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


//==============================================================================
/*
 */
class OscilloscopeModule : public Module, public juce::Timer
{
public:
    
    enum TriggerState {
        RUNNING,
        TRIGGERED,
    };
    
    OscilloscopeModule(double sampleRate, int buffersize);
    ~OscilloscopeModule();
    
    void paint (juce::Graphics& g) override;
    void resized() override;
    
    void process() override;
    virtual void configurePins() override;
    
    virtual void setCurrentLayer(Layer layer) override;
    
    virtual juce::String getCategory() override {
        return "Controls";
    }
    
    void timerCallback() override {
        repaint();
    }
    
    virtual Layer getLayer() override {
        return Layer::GUI;
    };
    
    virtual juce::String getDescription() override {
        return "A simple oscilloscope";
    }
    
    virtual void setLocked(bool locked) override {
        this->locked = locked;
        setInterceptsMouseClicks(locked, locked);
        // resizer->setEnabled(!locked);
        // resizer->setVisible(!locked);
        repaint();
    }
    
    
private:
    
    TriggerState state = RUNNING;

    int currentSample = 0;
    float buffer[1024];
    float xscale = 1;
    float yscale = 20;
    
    float triggerThreshold = 0;
    
    float periods = 1.0f;
    bool trigger = false;
    
    ResizableCornerComponent* resizer = nullptr;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OscilloscopeModule)
};
