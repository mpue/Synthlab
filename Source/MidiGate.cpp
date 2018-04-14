/*
  ==============================================================================

    MidiGate.cpp
    Created: 4 Apr 2018 3:48:23pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "MidiGate.h"
#include "Connection.h"
#include "MidiOut.h"

//==============================================================================
MidiGate::MidiGate()
{   
    setSize(120,140);
    
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(2,2);
    editable = false;
    prefab = true;
    
}

MidiGate::~MidiGate()
{
    
 
}

void MidiGate::configurePins() {

    
    Pin* p1 = new Pin(Pin::Type::VALUE);
    p1->direction = Pin::Direction::OUT;
    p1->listeners.push_back(this);
    p1->setName("G");
    addPin(Pin::Direction::OUT,p1);
    
    Pin* p2 = new Pin(Pin::Type::EVENT);
    p2->direction = Pin::Direction::OUT;
    p2->listeners.push_back(this);
    p2->setName("E");
    addPin(Pin::Direction::OUT,p2);
}

void MidiGate::paint(juce::Graphics &g) {
    Module::paint(g);
    g.drawImageAt(ImageCache::getFromMemory(BinaryData::midiin_png, BinaryData::midiin_pngSize),25,40);
}

void MidiGate::gateOn(int velocity, int note) {
    pins.at(0)->setValue(velocity);
    pins.at(0)->setNote(note);
    
    Event* e = new Event("gate on",Event::Type::GATE);
    e->setValue(velocity);
    e->setNote(note);
    
    for (int i = 0; i < pins.at(1)->connections.size();i++) {
        pins.at(1)->connections.at(i)->sendEvent(new Event(e));
    }
    
    delete e;
}

void MidiGate::gateOff(int note) {
    pins.at(0)->setValue(0);
    pins.at(0)->setNote(note);
    Event* e = new Event("gate off",Event::Type::GATE);
    e->setValue(0);
    e->setNote(note);
    
    for (int i = 0; i < pins.at(1)->connections.size();i++) {
        pins.at(1)->connections.at(i)->sendEvent(new Event(e));
    }
    
    delete e;
}
