/*
 ==============================================================================
 
 TriggerTimerModule.h
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
class TriggerTimerModule    : public Module, public Timer
{
public:
    TriggerTimerModule();
    ~TriggerTimerModule();
    
    void paint (juce::Graphics& g) override;
    
    virtual void configurePins() override;
    
    virtual juce::String getCategory() override {
        return "MIDI";
    }
    
    virtual void timerCallback() override;
    

    void trigger(int interval);
    virtual void eventReceived(Event *e) override;
    
    void setChannel(int channel);
    int getChannel();
    
    virtual juce::Array<juce::PropertyComponent*>& getProperties() override;
    virtual void createProperties() override;
    
private:
    
    struct ChannelListener : juce::Value::Listener
    {
        ChannelListener (juce::Value& v, TriggerTimerModule* g) : g(g),  value (v) { value.addListener (this); }
        ~ChannelListener()  {}  // no need to remove the listener
        
        void valueChanged (juce::Value& value) override {
            g->setChannel(value.toString().getIntValue());
        }
        TriggerTimerModule* g;
        juce::Value value;
    };
    
    juce::Value* channelValue;
    juce::PropertyComponent* channelProp;
    ChannelListener* channelListener;
    Pin* gatePin = nullptr;
    
    int channel = 1;
    int numTriggers = 5;
    int currentTrigger = 0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TriggerTimerModule)
};
