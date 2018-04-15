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
#include "AudioEngine/StereoDelay.h"

//==============================================================================
/*
*/
class DelayModule : public Module
{
public:
    DelayModule(double sampleRate, int buffersize);
    ~DelayModule();

    void paint (Graphics& g) override;

    void process() override;
    virtual void configurePins() override;
    virtual String getCategory() override {
        return "FX";
    }

private:

    Image* image;

    int currentSample = 0;
    float* bufferLeft;
    float* bufferRight;
    StereoDelay* delay;
    
    float delayL = 0.5;
    float delayR = 0.5;
    float feedback = 0.2;
    float mix = 0.5;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DelayModule)
};
