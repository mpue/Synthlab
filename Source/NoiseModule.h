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

#include "AudioEngine/WhiteNoise.h"

//==============================================================================
/*
*/
class NoiseModule : public Module
{
public:
    NoiseModule(double sampleRate, int buffersize);
    ~NoiseModule();

    void paint (Graphics& g) override;

    void setAmplitude(float amplitude);
    void process() override;
    

    
private:
    

    Image* image;
    WhiteNoise* oscillator;
    double sampleRate;
    int buffersize;
    int currentSample = 0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoiseModule)
};
