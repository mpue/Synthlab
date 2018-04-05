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
    // In your constructor, you should add any child components, and
    // initialise any special settings that your component needs.
   
    setSize(70,30);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(2,2);
    addPin(Pin::Direction::OUT);
    editable = false;
}

MidiGate::~MidiGate()
{
    
 
}

void MidiGate::paint(juce::Graphics &g) {
    Module::paint(g);
}

void MidiGate::gateOn(int velocity) {
    
    pins.at(0)->sendEvent(new Event("gate",Event::Type::GATE));
    
    /*
    for (int i = 0; i < pins.size();i++) {

        MidiOut* out;
        Module* module;
        
        if ((out = dynamic_cast<MidiOut*>(connections->at(i)->target)) != NULL) {
            out->gate(true, velocity);
        }
        if ((module = dynamic_cast<Module*>(connections->at(i)->target)) != NULL) {
            connections->at(i)->b->sendEvent(new Event("gate",Event::Type::GATE));
        }
        

    }
     */
}

void MidiGate::gateOff() {
    for (int i = 0; i < connections->size();i++) {
        
        MidiOut* out;
        
        if ((out = dynamic_cast<MidiOut*>(connections->at(i)->target)) != NULL) {
            out->gate(true, 0);
        }
        
    }
}
