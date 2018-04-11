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
    setSize(120,60);
    
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
}

void MidiGate::paint(juce::Graphics &g) {
    Module::paint(g);
}

void MidiGate::gateOn(int velocity) {
    pins.at(0)->setValue(velocity);
}

void MidiGate::gateOff() {
    pins.at(0)->setValue(0);
}
