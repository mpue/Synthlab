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

class WhiteNoise;

//==============================================================================
/*
*/
class NoiseModule : public Module
{
public:
    NoiseModule(double sampleRate, int buffersize);
    ~NoiseModule();

    void paint (juce::Graphics& g) override;

    void setAmplitude(float amplitude);
    void process() override;
    
    virtual void configurePins() override;
    virtual juce::String getCategory() override {
        return "Sound sources";
    }
    
private:
    

    juce::Image* image;
    WhiteNoise* oscillator;
    int currentSample = 0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoiseModule)
};
