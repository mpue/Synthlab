/*
  ==============================================================================

    InverterModule.cpp
    Created: 16 Apr 2018 5:09:40pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "InverterModule.h"



#include "Connection.h"


//==============================================================================
InverterModule::InverterModule()
{
    setSize(120,140);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(18,2);
    setName("Invert value");
    editable = false;
    prefab = true;
}

InverterModule::~InverterModule()
{
}

void InverterModule::configurePins() {
    Pin* p1 = new Pin(Pin::Type::VALUE);
    p1->direction = Pin::Direction::IN;
    p1->setName("A");
    
    Pin* p2 = new Pin(Pin::Type::VALUE);
    p2->direction = Pin::Direction::OUT;
    p2->setName("1/a");
    
    addPin(Pin::Direction::IN,p1);
    addPin(Pin::Direction::OUT,p2);

}

void InverterModule::paint(juce::Graphics &g) {
    Module::paint(g);
    g.drawImageAt(ImageCache::getFromMemory(BinaryData::inverter_png, BinaryData::inverter_pngSize),25,40);
}

void InverterModule::process() {
    
    float a = 1;
    
    if (pins.at(0)->connections.size() > 0) {
        a = pins.at(0)->connections.at(0)->getValue();
    }
        
    float b = 1.0f / a;
    
    pins.at(1)->setValue(b);
    
}
