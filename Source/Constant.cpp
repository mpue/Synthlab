/*
  ==============================================================================

    Constant
    Created: 4 Apr 2018 3:48:23pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "Constant.h"

using juce::Justification;
using juce::Value;
using juce::PropertyComponent;
using juce::SliderPropertyComponent;
using juce::Array;
using juce::String;

//==============================================================================
Constant::Constant()
{
   
    setSize(120,30);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(2,2);

    editable = false;
    prefab = true;

    createProperties();
    
}

void Constant::createProperties() {
    valueValue = new Value();
    valueListener = new ValueListener(*valueValue, this);
    
}

Constant::~Constant()
{
    delete valueValue;
    delete valueListener;
}

void Constant::configurePins(){
    Pin* p1 = new Pin(Pin::Type::VALUE);
    p1->direction = Pin::Direction::OUT;
    p1->setName("V");
    addPin(Pin::Direction::OUT,p1);

}

void Constant::paint(juce::Graphics &g) {
    Module::paint(g);
}

float Constant::getValue() {
    return value;
}

void Constant::setValue(float value) {
    this->value = value;
    valueValue->setValue(value);
    this->pins.at(0)->setValue(value);
    this->nameLabel->setText(name +" = "+String(value),juce::NotificationType::dontSendNotification);
}

void Constant::setEditing(bool editing) {
    /*
    if (editing) {
        if (!this->editing) {
            this->editing = true;
            nameLabel->setVisible(false);
            nameEditor->setVisible(true);
            nameEditor->setText(getName());
            setInterceptsMouseClicks(false,true);
            nameEditor->setWantsKeyboardFocus(true);
            nameEditor->setSelectAllWhenFocused(true);
            nameEditor->grabKeyboardFocus();
        }
    }
    else {
        if (this->editing) {
            this->editing = false;
            setName(nameEditor->getText());
            this->nameLabel->setText(name +" = "+String(value),juce::NotificationType::dontSendNotification);
            nameLabel->setVisible(true);
            nameEditor->setVisible(false);
            setInterceptsMouseClicks(false,false);
            nameEditor->setWantsKeyboardFocus(false);
        }
    }
     */

}

juce::Array<PropertyComponent*>& Constant::getProperties() {
    
    properties = juce::Array<PropertyComponent*>();
    valueProp = new SliderPropertyComponent(*valueValue,"Value",-65535,65535,0.1,1.0,true);

    properties.add(valueProp);
    
    return properties;
}

