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
#include "AudioEngine/Distortion.h"

//==============================================================================
/*
 */
class DistortionModule : public Module
{
public:
    DistortionModule(double sampleRate, int buffersize);
    DistortionModule();
    
    void paint (juce::Graphics& g) override;
    
    void process() override;
    virtual void configurePins() override;
    virtual juce::String getCategory() override {
        return "FX";
    }
    
private:
    
    juce::Image* image;
    
    int currentSample = 0;
    float* buffer;

    Distortion* distortion;
    
 
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DistortionModule)
};

