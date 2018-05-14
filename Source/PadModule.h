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
    
    void setHold(bool hold);
    bool isHold();
    
    virtual juce::Array<juce::PropertyComponent*>& getProperties() override;
    virtual void createProperties() override;
    
    virtual void buttonClicked (Button*) override;
    virtual void buttonStateChanged (Button*) override;
    
    virtual Layer getLayer() override {
        return Layer::GUI;
    };
    
    
    virtual void setCurrentLayer(Layer layer) override{
        currentLayer = layer;
        if (currentLayer == Layer::GUI) {
            nameLabel->setVisible(false);
        }
        else {
            nameLabel->setVisible(true);
        }
    }
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
    
    struct HoldListener : juce::Value::Listener
    {
        HoldListener (juce::Value& v, PadModule* g) : g(g),  value (v) { value.addListener (this); }
        ~HoldListener()  {}  // no need to remove the listener
        
        void valueChanged (juce::Value& value) override {
            g->setHold(value.toString().getIntValue() > 0);
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

    juce::Value* holdValue;
    juce::PropertyComponent* holdProp;
    HoldListener* holdListener;
    
    juce::TextButton* button;
    
    int note = 64;
    int channel = 1;
    bool holdMode = false;
    bool buttonDown = false;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PadModule)
};
