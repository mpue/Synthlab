/*
  ==============================================================================

    MidiGate.cpp
    Created: 4 Apr 2018 3:48:23pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "MidiOut.h"

//==============================================================================
MidiOut::MidiOut()
{
    // In your constructor, you should add any child components, and
    // initialise any special settings that your component needs.
   
    setSize(70,30);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(2,2);
    addPin(Pin::Direction::IN);
    editable = false;
}

MidiOut::~MidiOut()
{
    
 
}

void MidiOut::paint(juce::Graphics &g) {
    Module::paint(g);
}

void MidiOut::message(juce::MidiMessage *message) {
    deviceManager->getDefaultMidiOutput()->sendMessageNow(*message);
}

void MidiOut::gate(bool on, int velocity) {
    gateOn = on;
    this->velocity = velocity;

}

void MidiOut::setNote(int value) {
    this->note = value;
}

void MidiOut::setDeviceManager(juce::AudioDeviceManager* manager) {
    this->deviceManager = manager;
}
