/*
  ==============================================================================

    MidiGate.cpp
    Created: 4 Apr 2018 3:48:23pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "SawtoothModule.h"
#include "Connection.h"


//==============================================================================
SawtoothModule::SawtoothModule()
{
    // In your constructor, you should add any child components, and
    // initialise any special settings that your component needs.
   
    setSize(120,140);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(2,2);
    addPin(Pin::Direction::OUT);
    editable = false;
}

SawtoothModule::~SawtoothModule()
{
   
 
}

void SawtoothModule::paint(juce::Graphics &g) {
    Module::paint(g);
}

