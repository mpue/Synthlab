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
#include "AudioEngine/Random.h"

//==============================================================================
/*
*/
class RandomModule : public Module
{
public:
    RandomModule(double sampleRate, int buffersize);
    ~RandomModule();

    void paint (juce::Graphics& g) override;
    void setAmplitude(float amplitude);
    void process() override;
    virtual void eventReceived(Event *e) override;
    virtual void configurePins() override;
    
    virtual juce::String getCategory() override {
        return "Sound sources";
    }
    

private:
    float amplitude;
    Pin* gatePin;
    Pin* p1;
    Pin* p2;
    Pin* p3;
    Pin* p4;
    Pin* eventOutPin;
    juce::Random random;

    juce::Image* image;
    Synthlab::Random* oscillator;
    int currentSample = 0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RandomModule)
};
