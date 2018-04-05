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
   
    setSize(70,30);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(2,2);
    addPin(Pin::Direction::OUT);
    pins.at(0)->setName("P");
    editable = false;
}

MidiNote::~MidiNote()
{
    
 
}

void MidiNote::paint(juce::Graphics &g) {
    Module::paint(g);
}

void MidiNote::note(int note) {

    Event* e = new Event("note",Event::Type::NOTE);
    e->setValue(note);
    
    pins.at(0)->sendEvent(e);
    

}

