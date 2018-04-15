/*
  ==============================================================================

    Constant
    Created: 4 Apr 2018 3:48:23pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "Constant.h"


//==============================================================================
Constant::Constant()
{
   
    setSize(120,30);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(2,2);

    editable = false;
    prefab = true;
}

Constant::~Constant()
{
    addPin(Pin::Direction::OUT);
    pins.at(0)->setName("");
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
    this->pins.at(0)->setValue(value);
    this->nameLabel->setText(String(value),juce::NotificationType::dontSendNotification);
}



