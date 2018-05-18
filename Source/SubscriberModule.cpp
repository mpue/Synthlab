/*
 ==============================================================================
 
 SubscriberModule
 Created: 4 Apr 2018 3:48:23pm
 Author:  Matthias Pueski
 
 ==============================================================================
 */

#include "../JuceLibraryCode/JuceHeader.h"
#include "SubscriberModule.h"
#include "MessageBus/MessageBus.h"
#include "MessageBus/Topic.h"

using juce::Justification;
using juce::Value;
using juce::PropertyComponent;
using juce::SliderPropertyComponent;
using juce::Array;
using juce::String;

//==============================================================================
SubscriberModule::SubscriberModule()
{
    
    setSize(120,30);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(2,2);
    
    editable = false;
    prefab = true;
    
    createProperties();
    
}

void SubscriberModule::createProperties() {
    topicValue = new Value();
    topicListener = new TopicListener(*topicValue, this);
}

SubscriberModule::~SubscriberModule()
{
    delete topicValue;
    delete topicListener;
}

void SubscriberModule::configurePins(){
    Pin* p1 = new Pin(Pin::Type::VALUE);
    p1->direction = Pin::Direction::OUT;
    p1->setName("V");
    addPin(Pin::Direction::OUT, p1);
    
}

void SubscriberModule::paint(juce::Graphics &g) {
    Module::paint(g);
}

String SubscriberModule::getTopic() {
    return topic;
}

void SubscriberModule::setTopic(juce::String topic) {
    this->topic = topic;
    topicValue->setValue(topic);
    this->nameLabel->setText(name +" = "+String(topic),juce::NotificationType::dontSendNotification);
    MessageBus::getInstance()->addListener(topic, this);
}

void SubscriberModule::setEditing(bool editing) {
    
    
}

void SubscriberModule::topicChanged(Topic *t) {
    if (t->getName() == topic) {
        pins.at(0)->setValue(t->getValue());
    }
}

juce::Array<PropertyComponent*>& SubscriberModule::getProperties() {
    
    properties = juce::Array<PropertyComponent*>();
    topicProp = new TextPropertyComponent(*topicValue,"Topic",16, false,true);
    
    properties.add(topicProp);
    
    return properties;
}

