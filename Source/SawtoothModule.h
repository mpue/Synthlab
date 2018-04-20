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
#include "AudioEngine/Sawtooth.h"

//==============================================================================
/*
*/
class SawtoothModule : public Module
{
public:
    SawtoothModule(double sampleRate, int buffersize);
    ~SawtoothModule();

    void paint (Graphics& g) override;

    void setPitch(int pitch);
    void setAmplitude(float amplitude);
    void setFine(float fine);
    void process() override;
    virtual void configurePins() override;
    
    virtual String getCategory() override {
        return "Sound sources";
    }
    virtual String getDescription() override {
        return BinaryData::oscillator_txt;
    }
    
    bool isMono();
    void setMono(bool value);
    
    virtual juce::Array<PropertyComponent*>& getProperties() override;
    virtual void createProperties() override;
    
    
private:
    
    struct IsMonoListener : Value::Listener
    {
        IsMonoListener (Value& v, SawtoothModule* mod) : mod(mod),  value (v) { value.addListener (this); }
        ~IsMonoListener()  {}  // no need to remove the listener
        
        void valueChanged (Value& value) override {
            mod->setMono(value.toString().getIntValue() > 0);
        }
        SawtoothModule* mod;
        Value value;
    };
    
    Value* isMonoValue;
    PropertyComponent* isMonoProp;
    IsMonoListener* isMonoListener;
    
    int pitch = 0;
    float amplitude;
    float fine = 0;
    Image* image;
    Sawtooth* monoOscillator;
    Sawtooth* oscillator[128];
    int currentSample = 0;
    bool mono;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SawtoothModule)
};
