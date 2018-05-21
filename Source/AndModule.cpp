/*
 ==============================================================================
 
 AndModule.cpp
 Created: 16 Apr 2018 12:42:07pm
 AuthAnd:  Matthias Pueski
 
 ==============================================================================
 */

#include "AndModule.h"
#include "../JuceLibraryCode/JuceHeader.h"

#include "Connection.h"

using juce::Justification;
using juce::ImageCache;

//==============================================================================
AndModule::AndModule()
{
    setSize(110,70);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(18,2);
    setName("And");
    nameLabel->setVisible(false);
    editable = false;
    prefab = true;
}

AndModule::~AndModule()
{
}

void AndModule::configurePins() {
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

void AndModule::paint(juce::Graphics &g) {
    Module::paint(g);
    g.drawImageAt(ImageCache::getFromMemory(BinaryData::logic_and_png, BinaryData::logic_and_pngSize),getWidth()/2 - 32,getHeight() / 2  - 32);
}

void AndModule::process() {
    
    float a = 0;
    float b = 0;
    float c = 0;
    
    if (pins.at(0)->getConnections().size() > 0) {
        a = pins.at(0)->getConnections().at(0)->getValue();
    }
    if (pins.at(1)->getConnections().size() > 0) {
        b = pins.at(1)->getConnections().at(0)->getValue();
    }
    
    if (a > 0 && b > 0) {
        c = 1;
    }
    else {
        c = 0;
    }
    
    pins.at(2)->setValue(c);
    
}

