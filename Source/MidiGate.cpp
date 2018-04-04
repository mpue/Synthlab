/*
  ==============================================================================

    MidiGate.cpp
    Created: 4 Apr 2018 3:48:23pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "MidiGate.h"

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

void MidiGate::gate() {
    Logger::writeToLog(getName() + " : Gate received.");
}

