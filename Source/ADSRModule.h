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
#include "AudioEngine/ADSR.h"

//==============================================================================
/*
*/
class ADSRModule : public Module
{
public:
    ADSRModule(double sampleRate, int buffersize);
    ~ADSRModule();

    void paint (Graphics& g) override;

    void process() override;
    virtual void configurePins() override;
    
    virtual String getCategory() override {
        return "Sound sources";
    }
    
    void setAttack(float attack);
    void setDecay(float decay);
    void setSustain(float sustain);
    void setRelease(float release);
    
    float getAttack();
    float getDecay();
    float getSustain();
    float getRelease();
        
private:
    

    Image* image;
    ADSR* envelope;
    double sampleRate;
    int buffersize;
    int currentSample = 0;
    
    float attack = 0;
    float decay = 0;
    float sustain = 0.8;
    float release = 0;
    
    float value = 0;
    bool gate = false;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ADSRModule)
};
