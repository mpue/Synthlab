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

//==============================================================================
/*
 */
class MixerModule    : public Module
{
public:
    MixerModule(float sampleRate, int buffersize);
    ~MixerModule();
    
    
    void paint (juce::Graphics& g) override;
    virtual void process() override;
    virtual void configurePins() override;
    void setInputs(int numInputs);
    virtual juce::String getCategory() override {
        return "Audio";
    }
    virtual juce::Array<juce::PropertyComponent*>& getProperties() override;
    virtual void createProperties() override;
private:
    
    struct InputsListener : juce::Value::Listener
    {
        InputsListener (juce::Value& v, MixerModule* m) : m(m),  value (v) { value.addListener (this); }
        ~InputsListener()  {}  // no need to remove the listener
        
        void valueChanged (juce::Value& value) override {
            m->setInputs(value.toString().getIntValue());
        }
        MixerModule* m;
        juce::Value value;
    };
    
    juce::Value* inputsValue;
    juce::PropertyComponent* inputsProp;
    InputsListener* inputsListener;
    
    int numInputs = 3;
    Pin* out;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MixerModule)
};
