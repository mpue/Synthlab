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
#include "MessageBus/BusListener.h"

class MessageBus;
class Topic;


//==============================================================================
/*
 */
class SubscriberModule    : public Module, public BusListener
{
public:
    SubscriberModule();
    ~SubscriberModule();
    
    void paint (juce::Graphics& g) override;
    
    void setTopic(juce::String value);
    juce::String getTopic();
    virtual void configurePins() override;
    
    virtual void setName(juce::String name) override {
        //Module::setName(name);
        juce::Component::setName(name);
        this->name = name;
        this->nameLabel->setText(name +" = "+juce::String(topic),juce::NotificationType::dontSendNotification);
    }
    
    virtual void topicChanged(Topic* t) override;
    
    virtual void setEditing(bool editing) override;
    
    virtual juce::String getName()  {
        return name;
    }
    
    virtual juce::String getCategory() override {
        return "Math";
    }
    
    struct TopicListener : juce::Value::Listener
    {
        TopicListener (juce::Value& v, SubscriberModule* c) : c(c),  value (v) { value.addListener (this); }
        ~TopicListener()  {}  // no need to remove the listener
        
        void valueChanged (juce::Value& value) override {
            c->setTopic(value.toString());
        }
        SubscriberModule* c;
        juce::Value value;
    };
    
    virtual juce::Array<juce::PropertyComponent*>& getProperties() override;
    virtual void createProperties() override;
    
private:
    
    juce::Value* topicValue;
    juce::PropertyComponent* topicProp;
    TopicListener* topicListener;
    
    juce::String topic;
    juce::String name;
    bool editing = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SubscriberModule)
};
