/*
 ==============================================================================
 
 NotModule.cpp
 Created: 16 Apr 2018 12:42:07pm
 AuthNot:  Matthias Pueski
 
 ==============================================================================
 */

#include "NotModule.h"
#include "../JuceLibraryCode/JuceHeader.h"

#include "Connection.h"

using juce::Justification;
using juce::ImageCache;

//==============================================================================
NotModule::NotModule()
{
    setSize(120,140);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(18,2);
    setName("Not");
    editable = false;
    prefab = true;
}

NotModule::~NotModule()
{
}

void NotModule::configurePins() {
    Pin* p1 = new Pin(Pin::Type::VALUE);
    p1->direction = Pin::Direction::IN;
    p1->setName("A");
    
    Pin* p2 = new Pin(Pin::Type::VALUE);
    p2->direction = Pin::Direction::IN;
    p2->setName("B");
    
    
    addPin(Pin::Direction::IN,p1);
    addPin(Pin::Direction::OUT,p2);
}

void NotModule::paint(juce::Graphics &g) {
    Module::paint(g);
    g.drawImageAt(ImageCache::getFromMemory(BinaryData::logic_not_png, BinaryData::logic_not_pngSize),25,40);
}

void NotModule::process() {
    
    float a = 0;
    float b = 0;
    
    if (pins.at(0)->getConnections().size() > 0) {
        a = pins.at(0)->getConnections().at(0)->getValue();
    }
    
    if (a > 0 ) {
        b = 0;
    }
    else {
        b = 1;
    }
    
    pins.at(1)->setValue(b);
}

