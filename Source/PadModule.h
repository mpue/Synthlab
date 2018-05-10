/*
  ==============================================================================

    PadModule.h
    Created: 9 May 2018 11:49:06pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"

//==============================================================================
/*
 */
class PadModule    : public Module, public juce::Button::Listener
{
public:
    PadModule();
    ~PadModule();
    
    void paint (juce::Graphics& g) override;
    
    void gateOn(int velocity, int note);
    void gateOff(int note);
    
    virtual void configurePins() override;
    
    virtual juce::String getCategory() override {
        return "Controls";
    }
    
    void setChannel(int channel);
    int getChannel();
    
    void setNote(int note);
    int getNote();
    
    virtual juce::Array<juce::PropertyComponent*>& getProperties() override;
    virtual void createProperties() override;
    
    virtual void buttonClicked (Button*) override;
    virtual void buttonStateChanged (Button*) override;
    
    virtual Layer getLayer() override {
        return Layer::GUI;
    };
    
private:
    
    struct ChannelListener : juce::Value::Listener
    {
        ChannelListener (juce::Value& v, PadModule* g) : g(g),  value (v) { value.addListener (this); }
        ~ChannelListener()  {}  // no need to remove the listener
        
        void valueChanged (juce::Value& value) override {
            g->setChannel(value.toString().getIntValue());
        }
        PadModule* g;
        juce::Value value;
    };
    
    struct NoteListener : juce::Value::Listener
    {
        NoteListener (juce::Value& v, PadModule* g) : g(g),  value (v) { value.addListener (this); }
        ~NoteListener()  {}  // no need to remove the listener
        
        void valueChanged (juce::Value& value) override {
            g->setNote(value.toString().getIntValue());
        }
        PadModule* g;
        juce::Value value;
    };
    
    juce::Value* channelValue;
    juce::PropertyComponent* channelProp;
    ChannelListener* channelListener;
    
    juce::Value* noteValue;
    juce::PropertyComponent* noteProp;
    NoteListener* noteListener;
    
    juce::TextButton* button;
    
    int note = 64;
    int channel = 1;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PadModule)
};
