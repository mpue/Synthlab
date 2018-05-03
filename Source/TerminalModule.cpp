/*
 ==============================================================================
 
 TerminalModule
 Created: 4 Apr 2018 3:48:23pm
 Author:  Matthias Pueski
 
 ==============================================================================
 */

#include "../JuceLibraryCode/JuceHeader.h"
#include "TerminalModule.h"


//==============================================================================
TerminalModule::TerminalModule()
{
    
    setSize(120,30);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(2,2);
    
    editable = false;
    prefab = true;
    
    createProperties();
    
}

void TerminalModule::createProperties() {
    valueValue = new Value();
    valueListener = new ValueListener(*valueValue, this);
    
}

TerminalModule::~TerminalModule()
{
    delete valueValue;
    delete valueListener;
}

void TerminalModule::configurePins(){
    Pin* p1 = new Pin(Pin::Type::VALUE);
    p1->direction = Pin::Direction::OUT;
    p1->setName("V");
    addPin(Pin::Direction::OUT,p1);
    
}

void TerminalModule::paint(juce::Graphics &g) {
    Module::paint(g);
}

float TerminalModule::getValue() {
    return value;
}

void TerminalModule::setValue(float value) {
    this->value = value;
    valueValue->setValue(value);
    this->pins.at(0)->setValue(value);
    this->nameLabel->setText(name +" = "+String(value),juce::NotificationType::dontSendNotification);
}


juce::Array<PropertyComponent*>& TerminalModule::getProperties() {
    
    properties = juce::Array<PropertyComponent*>();
    valueProp = new SliderPropertyComponent(*valueValue,"Value",-65535,65535,0.1,1.0,true);
    
    properties.add(valueProp);
    
    return properties;
}


