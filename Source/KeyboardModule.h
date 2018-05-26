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
class KeyboardModule    : public Module, public juce::MidiKeyboardStateListener
{
public:
    KeyboardModule();
    ~KeyboardModule();
    
    void paint (juce::Graphics& g) override;
    
    void setImage(String path);
    String getImage();
    
    void setChannel(int channel);
    int getChannel();
    
    virtual void setCurrentLayer(Layer layer) override;
    
    virtual void configurePins() override;
    
    virtual juce::String getCategory() override {
        return "Controls";
    }
    
    void gateOn(int velocity, int note);
    void gateOff(int note);
    
    virtual void handleNoteOn (MidiKeyboardState* source,int midiChannel, int midiNoteNumber, float velocity) override;
    virtual void handleNoteOff (MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity) override;
    
    virtual Layer getLayer() override {
        return Layer::GUI;
    };
    
    virtual void setLocked(bool locked) override {
        this->locked = locked;
        this->keyboard->setEnabled(locked);
        setInterceptsMouseClicks(locked, locked);
        resizer->setEnabled(!locked);
        resizer->setVisible(!locked);
        repaint();
    }
    
    void resized() override;
    
private:
    
    struct ChannelListener : juce::Value::Listener
    {
        ChannelListener (juce::Value& v, KeyboardModule* k) : k(k),  value (v) { value.addListener (this); }
        ~ChannelListener()  {}  // no need to remove the listener
        
        void valueChanged (juce::Value& value) override {
            k->setChannel(value.toString().getIntValue());
        }
        KeyboardModule* k;
        juce::Value value;
    };
    
    juce::Value* channelValue;
    juce::PropertyComponent* channelProp;
    ChannelListener* channelListener;
    
    juce::MidiKeyboardState state;
    juce::MidiKeyboardComponent::Orientation orientation = juce::MidiKeyboardComponent::Orientation::horizontalKeyboard;
    
    virtual juce::Array<juce::PropertyComponent*>& getProperties() override;
    virtual void createProperties() override;
    
private:
    MidiKeyboardComponent* keyboard = nullptr;
    ResizableCornerComponent* resizer = nullptr;
    int channel = 1;
    bool editing = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KeyboardModule)
};
