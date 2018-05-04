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

    void paint (juce::Graphics& g) override;

    void gateOn(int velocity, int note);
    void gateOff(int note);
    
    virtual void configurePins() override;
    
    virtual juce::String getCategory() override {
        return "MIDI";
    }
    
    void setChannel(int channel);
    int getChannel();
    
    virtual juce::Array<juce::PropertyComponent*>& getProperties() override;
    virtual void createProperties() override;
    
private:
    
    struct ChannelListener : juce::Value::Listener
    {
        ChannelListener (juce::Value& v, MidiGate* g) : g(g),  value (v) { value.addListener (this); }
        ~ChannelListener()  {}  // no need to remove the listener
        
        void valueChanged (juce::Value& value) override {
            g->setChannel(value.toString().getIntValue());
        }
        MidiGate* g;
        juce::Value value;
    };
    
    juce::Value* channelValue;
    juce::PropertyComponent* channelProp;
    ChannelListener* channelListener;

    int channel = 1;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiGate)
};
