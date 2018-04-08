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
    setSize(120,140);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(18,2);
    
    setName("Sawtooth");
    Pin* p1 = new Pin(Pin::Type::EVENT);
    p1->direction = Pin::Direction::IN;
    p1->listeners.push_back(this);
    p1->setName("P");
    
    Pin* p2 = new Pin(Pin::Type::EVENT);
    p2->direction = Pin::Direction::IN;
    p2->listeners.push_back(this);
    p2->setName("F");

    Pin* p3 = new Pin(Pin::Type::EVENT);
    p3->direction = Pin::Direction::IN;
    p3->listeners.push_back(this);
    p3->setName("A");
    
    Pin* p4 = new Pin(Pin::Type::AUDIO);
    p4->direction = Pin::Direction::OUT;
    p4->listeners.push_back(this);
    p4->setName("Out");
    
    addPin(Pin::Direction::IN,p1);
    addPin(Pin::Direction::IN,p2);
    addPin(Pin::Direction::IN,p3);
    addPin(Pin::Direction::OUT,p4);
    
    editable = false;
}

SawtoothModule::~SawtoothModule()
{
}

void SawtoothModule::paint(juce::Graphics &g) {
    Module::paint(g);
    g.drawImageAt(ImageCache::getFromMemory(BinaryData::sawtooth_png, BinaryData::sawtooth_pngSize),25,40);
}


void SawtoothModule::setPitch(int pitch) {
    this->pitch = pitch;
}

void SawtoothModule::setFine(float fine) {
    this->fine = fine;
}

void SawtoothModule::setAmplitude(float amplitude) {
    this->amplitude = amplitude;
}
