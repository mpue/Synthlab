/*
 ==============================================================================
 
 PublisherModule
 Created: 4 Apr 2018 3:48:23pm
 Author:  Matthias Pueski
 
 ==============================================================================
 */

#include "../JuceLibraryCode/JuceHeader.h"
#include "PublisherModule.h"
#include "MessageBus/MessageBus.h"

using juce::Justification;
using juce::Value;
using juce::PropertyComponent;
using juce::SliderPropertyComponent;
using juce::Array;
using juce::String;

//==============================================================================
PublisherModule::PublisherModule()
{
    
    setSize(120,30);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(2,2);
    
    editable = false;
    prefab = true;
    
    createProperties();
    
}

void PublisherModule::createProperties() {
    topicValue = new Value();
    topicListener = new TopicListener(*topicValue, this);
}

PublisherModule::~PublisherModule()
{
    delete topicValue;
    delete topicListener;
}

void PublisherModule::configurePins(){
    Pin* p1 = new Pin(Pin::Type::VALUE);
    p1->direction = Pin::Direction::IN;
    p1->setName("V");
    addPin(Pin::Direction::IN,p1);
    
}

void PublisherModule::paint(juce::Graphics &g) {
    Module::paint(g);
}

String PublisherModule::getTopic() {
    return topic;
}

void PublisherModule::setTopic(juce::String topic) {
    this->topic = topic;
    topicValue->setValue(topic);
    this->nameLabel->setText(name +" = "+String(topic),juce::NotificationType::dontSendNotification);
}

void PublisherModule::setEditing(bool editing) {

    
}

void PublisherModule::process() {
    
    if (pins.at(0)->getConnections().size() >= 1) {
        float value = pins.at(0)->getConnections().at(0)->getValue();
        MessageBus::getInstance()->updateTopic(topic,value);
    }
   
}

juce::Array<PropertyComponent*>& PublisherModule::getProperties() {
    
    properties = juce::Array<PropertyComponent*>();
    topicProp = new TextPropertyComponent(*topicValue,"Topic",16, false,true);
    
    properties.add(topicProp);
    
    return properties;
}

