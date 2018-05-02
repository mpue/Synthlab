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
    
    
    void paint (Graphics& g) override;
    virtual void process() override;
    virtual void configurePins() override;
    void setInputs(int numInputs);
    virtual String getCategory() override {
        return "Audio";
    }
    virtual juce::Array<PropertyComponent*>& getProperties() override;
    virtual void createProperties() override;
private:
    
    struct InputsListener : Value::Listener
    {
        InputsListener (Value& v, MixerModule* m) : m(m),  value (v) { value.addListener (this); }
        ~InputsListener()  {}  // no need to remove the listener
        
        void valueChanged (Value& value) override {
            m->setInputs(value.toString().getIntValue());
        }
        MixerModule* m;
        Value value;
    };
    
    Value* inputsValue;
    PropertyComponent* inputsProp;
    InputsListener* inputsListener;
    
    int numInputs = 3;
    Pin* out;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MixerModule)
};
