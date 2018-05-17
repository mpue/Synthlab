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
#include "AudioEngine/FractionalDelayBuffer.h"
#include "AudioEngine/Sine.h"

//==============================================================================
/*
 */
class ChorusModule : public Module
{
public:
    ChorusModule(double sampleRate, int buffersize);
    ChorusModule();
    
    void paint (juce::Graphics& g) override;
    
    void process() override;
    virtual void configurePins() override;
    virtual juce::String getCategory() override {
        return "FX";
    }
    
private:
    
    
    int currentSample = 0;

    float leftDelayTime, rightDelayTime = 10;
    Sine* leftOsc = nullptr;
    Sine* rightOsc = nullptr;
    FractionalDelayBuffer* leftBuffer = nullptr;
    FractionalDelayBuffer* rightBuffer = nullptr;
    float modulation = 0.5f;
    float delay = 0.005f;
    float feedback = 0.1f;
    float leftMod = 0.1f;
    float rightMod = 0.1f;
    float mix = 0.5f;
    
    
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChorusModule)
};

