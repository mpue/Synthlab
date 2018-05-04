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
class Constant    : public Module
{
public:
    Constant();
    ~Constant();

    void paint (juce::Graphics& g) override;

    void setValue(float value);
    float getValue();
    virtual void configurePins() override;

    virtual void setName(juce::String name) override {
        //Module::setName(name);
        juce::Component::setName(name);
        this->name = name;
        this->nameLabel->setText(name +" = "+juce::String(value),juce::NotificationType::dontSendNotification);
    }
    
    virtual void setEditing(bool editing) override;
    
    virtual juce::String getName()  {
        return name;
    }
    
    virtual juce::String getCategory() override {
        return "Math";
    }
    
    struct ValueListener : juce::Value::Listener
    {
        ValueListener (juce::Value& v, Constant* c) : c(c),  value (v) { value.addListener (this); }
        ~ValueListener()  {}  // no need to remove the listener
        
        void valueChanged (juce::Value& value) override {
            c->setValue(value.toString().getFloatValue());
        }
        Constant* c;
        juce::Value value;
    };
    
    virtual juce::Array<juce::PropertyComponent*>& getProperties() override;
    virtual void createProperties() override;
    
private:
    
    juce::Value* valueValue;
    juce::PropertyComponent* valueProp;
    ValueListener* valueListener;
    
    float value = 0;
    juce::String name;
    bool editing = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Constant)
};
