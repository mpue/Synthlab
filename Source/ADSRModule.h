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
    
    bool isMono();
    void setMono(bool value);
    
    void eventReceived(Event *e) override;
    
    virtual String getDescription() override {
        return BinaryData::adsr_txt;
    }
    
    virtual juce::Array<PropertyComponent*>& getProperties() override;
    virtual void createProperties() override;
    
private:
    
    struct IsMonoListener : Value::Listener
    {
        IsMonoListener (Value& v, ADSRModule* mod) : mod(mod),  value (v) { value.addListener (this); }
        ~IsMonoListener()  {}  // no need to remove the listener
        
        void valueChanged (Value& value) override {
            mod->setMono(value.toString().getIntValue() > 0);
        }
        ADSRModule* mod;
        Value value;
    };
    
    Value* isMonoValue;
    PropertyComponent* isMonoProp;
    IsMonoListener* isMonoListener;

    Image* image;
    ADSR* envelope;
    ADSR* envelopes[128];
    int currentSample = 0;
    
    float attack = 0;
    float decay = 0;
    float sustain = 0.8;
    float release = 0;
    
    float value = 0;
    bool gate[128] = {false};
    int voices = 0;
    
    int lastNote = 0;
    int lastVelocity = 0;
    
    bool mono = false;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ADSRModule)
};
