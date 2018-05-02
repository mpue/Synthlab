/*
 ==============================================================================
 
 MidiGate.cpp
 Created: 4 Apr 2018 3:48:23pm
 Author:  Matthias Pueski
 
 ==============================================================================
 */

#include "../JuceLibraryCode/JuceHeader.h"
#include "AuxOut.h"
#include "Connection.h"


//==============================================================================
AuxOut::AuxOut()
{
    setSize(120,140);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(18,2);
    setName("Aux out");
    
    editable = false;
    prefab = true;
}

AuxOut::~AuxOut()
{
    
    
}

void AuxOut::configurePins() {
    Pin* p1 = new Pin(Pin::Type::AUDIO);
    p1->direction = Pin::Direction::IN;
    p1->setName("L");
    
    Pin* p2 = new Pin(Pin::Type::AUDIO);
    p2->direction = Pin::Direction::IN;
    p2->setName("R");
    
    addPin(Pin::Direction::IN,p1);
    addPin(Pin::Direction::IN,p2);
    
}

void AuxOut::paint(juce::Graphics &g) {
    Module::paint(g);
    g.drawImageAt(ImageCache::getFromMemory(BinaryData::aux_png, BinaryData::aux_pngSize),25,40);
}

void AuxOut::process() {
    
}

void AuxOut::setChannelIndex(int index) {
    this->channelIndex = index;
}

int AuxOut::getChannelIndex() {
    return channelIndex;
}
