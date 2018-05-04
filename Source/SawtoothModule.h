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

    void paint (juce::Graphics& g) override;

    void setPitch(int pitch);
    void setAmplitude(float amplitude);
    void setFine(float fine);
    void process() override;
    virtual void configurePins() override;
    
    virtual juce::String getCategory() override {
        return "Sound sources";
    }
    virtual juce::String getDescription() override {
        return BinaryData::oscillator_txt;
    }
    
    bool isMono();
    void setMono(bool value);
    
    virtual juce::Array<juce::PropertyComponent*>& getProperties() override;
    virtual void createProperties() override;
    
    
private:
    
    struct IsMonoListener : juce::Value::Listener
    {
        IsMonoListener (juce::Value& v, SawtoothModule* mod) : mod(mod),  value (v) { value.addListener (this); }
        ~IsMonoListener()  {}  // no need to remove the listener
        
        void valueChanged (juce::Value& value) override {
            mod->setMono(value.toString().getIntValue() > 0);
        }
        SawtoothModule* mod;
        juce::Value value;
    };
    
    juce::Value* isMonoValue;
    juce::PropertyComponent* isMonoProp;
    IsMonoListener* isMonoListener;
    
    juce::AudioSampleBuffer* out = nullptr;
    float frequencies[128] = {0};
    int pitch = 0;
    float amplitude;
    float fine = 0;
    juce::Image* image;
    Sawtooth* monoOscillator;
    Sawtooth* oscillator[128];
    int currentSample = 0;
    bool mono;

    Pin* gatePin = nullptr;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SawtoothModule)
};
