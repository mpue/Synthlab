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
class MidiGate    : public Module
{
public:
    MidiGate();
    ~MidiGate();

    void paint (Graphics& g) override;

    void gateOn(int velocity, int note);
    void gateOff(int note);
    
    virtual void configurePins() override;
    
    virtual String getCategory() override {
        return "MIDI";
    }
    
    void setChannel(int channel);
    int getChannel();
    
    virtual juce::Array<PropertyComponent*>& getProperties() override;
    virtual void createProperties() override;
    
private:
    
    struct ChannelListener : Value::Listener
    {
        ChannelListener (Value& v, MidiGate* g) : g(g),  value (v) { value.addListener (this); }
        ~ChannelListener()  {}  // no need to remove the listener
        
        void valueChanged (Value& value) override {
            g->setChannel(value.toString().getIntValue());
        }
        MidiGate* g;
        Value value;
    };
    
    Value* channelValue;
    PropertyComponent* channelProp;
    ChannelListener* channelListener;

    int channel = 1;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiGate)
};
