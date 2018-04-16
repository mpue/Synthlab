/*
  ==============================================================================

    AddValueModule.cpp
    Created: 16 Apr 2018 12:42:07pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "AddValueModule.h"
#include "../JuceLibraryCode/JuceHeader.h"

#include "Connection.h"


//==============================================================================
AddValueModule::AddValueModule()
{
    setSize(120,140);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(18,2);
    setName("Add value");
    editable = false;
    prefab = true;
}

AddValueModule::~AddValueModule()
{
}

void AddValueModule::configurePins() {
    Pin* p1 = new Pin(Pin::Type::VALUE);
    p1->direction = Pin::Direction::IN;
    p1->setName("A");
    
    Pin* p2 = new Pin(Pin::Type::VALUE);
    p2->direction = Pin::Direction::IN;
    p2->setName("B");
    
    Pin* p3 = new Pin(Pin::Type::VALUE);
    p3->direction = Pin::Direction::OUT;
    p3->setName("C");
    
    addPin(Pin::Direction::IN,p1);
    addPin(Pin::Direction::IN,p2);
    addPin(Pin::Direction::OUT,p3);
}

void AddValueModule::paint(juce::Graphics &g) {
    Module::paint(g);
    g.drawImageAt(ImageCache::getFromMemory(BinaryData::adder_png, BinaryData::adder_pngSize),25,40);
}

void AddValueModule::process() {
    
    float a = 0;
    float b = 0;
    float c = 0;
    
    if (pins.at(0)->connections.size() > 0) {
        a = pins.at(0)->connections.at(0)->getValue();
    }
    if (pins.at(1)->connections.size() > 0) {
        b = pins.at(1)->connections.at(0)->getValue();
    }
    
    c = a + b;
    
    pins.at(2)->setValue(c);
    
}

