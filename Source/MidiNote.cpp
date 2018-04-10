/*
  ==============================================================================

    MidiGate.cpp
    Created: 4 Apr 2018 3:48:23pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "MidiNote.h"
#include "Connection.h"


//==============================================================================
MidiNote::MidiNote()
{
    // In your constructor, you should add any child components, and
    // initialise any special settings that your component needs.
   
    setSize(120,60);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(2,2);

    
    editable = false;
     prefab = true;
}

MidiNote::~MidiNote()
{
    
 
}

void MidiNote::configurePins() {
    addPin(Pin::Direction::OUT);
    
    pins.at(0)->setName("P");
    
    
    Pin* p2 = new Pin(Pin::Type::VALUE);
    p2->direction = Pin::Direction::OUT;
    p2->listeners.push_back(this);
    p2->setName("V");
    addPin(Pin::Direction::OUT,p2);
}

void MidiNote::paint(juce::Graphics &g) {
    Module::paint(g);
}

void MidiNote::note(int note) {

    Event* e = new Event("note",Event::Type::NOTE);
    e->setValue(note);
    
    pins.at(0)->sendEvent(e);
    
    pins.at(1)->setValue(440 * pow(2.0,(note-69.0)/12.0));
}

