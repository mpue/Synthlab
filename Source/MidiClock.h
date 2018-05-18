/*
 ==============================================================================
 
 MidiClock.h
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
class MidiClock    : public Module, public Timer
{
public:
    MidiClock();
    ~MidiClock();
    
    void paint (juce::Graphics& g) override;
    
    virtual void configurePins() override;
    
    virtual juce::String getCategory() override {
        return "MIDI";
    }
    
    virtual void timerCallback() override;
    
    void clock(MidiMessage message);
    void midiStart();
    void midiStop();
    
    void setChannel(int channel);
    int getChannel();
    
    virtual juce::Array<juce::PropertyComponent*>& getProperties() override;
    virtual void createProperties() override;
    
    bool isInternal();
    void setInternal(bool isInternal);
    
    void setTempo(float tempo);
    float getTempo();
    
private:
    
    struct ChannelListener : juce::Value::Listener
    {
        ChannelListener (juce::Value& v, MidiClock* g) : g(g),  value (v) { value.addListener (this); }
        ~ChannelListener()  {}  // no need to remove the listener
        
        void valueChanged (juce::Value& value) override {
            g->setChannel(value.toString().getIntValue());
        }
        MidiClock* g;
        juce::Value value;
    };
    
    struct InternalClockListener : juce::Value::Listener
    {
        InternalClockListener (juce::Value& v, MidiClock* g) : g(g),  value (v) { value.addListener (this); }
        ~InternalClockListener()  {}  // no need to remove the listener
        
        void valueChanged (juce::Value& value) override {
            g->setInternal(value.toString().getIntValue() > 0);
        }
        MidiClock* g;
        juce::Value value;
    };
    
    struct TempoListener : juce::Value::Listener
    {
        TempoListener (juce::Value& v, MidiClock* g) : g(g),  value (v) { value.addListener (this); }
        ~TempoListener()  {}  // no need to remove the listener
        
        void valueChanged (juce::Value& value) override {
            g->setTempo(value.toString().getFloatValue());
        }
        MidiClock* g;
        juce::Value value;
    };
    
    juce::Value* channelValue;
    juce::PropertyComponent* channelProp;
    ChannelListener* channelListener;
    
    juce::Value* internalValue;
    juce::PropertyComponent* internalProp;
    InternalClockListener* internalListener;
    
    juce::Value* tempoValue;
    juce::PropertyComponent* tempoProp;
    TempoListener* tempoListener;
    
    int channel = 1;
    
    bool internalClock = false;
    float tempo = 120.0f;
    
    bool running = false;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiClock)
};
