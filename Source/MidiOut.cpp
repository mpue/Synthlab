 /*
  ==============================================================================

    MidiGate.cpp
    Created: 4 Apr 2018 3:48:23pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "MidiOut.h"
#include "AudioManager.h"

using juce::ImageCache;
using juce::MidiMessage;
using juce::uint8;
using juce::Logger;

//==============================================================================
MidiOut::MidiOut()
{
    // In your constructor, you should add any child components, and
    // initialise any special settings that your component needs.
   
    setSize (120, 140);
    // nameLabel->setJustificationType (Justification::left);
    // nameLabel->setTopLeftPosition(2,2);
    

    
    editable = false;
    prefab = true;
    this->deviceManager = AudioManager::getInstance()->getDeviceManager();
    // deviceManager->setDefaultMidiOutput("");
}

MidiOut::~MidiOut()
{
    
 
}

void MidiOut::configurePins() {
    Pin* p1 = new Pin(Pin::Type::EVENT);
    p1->direction = Pin::Direction::IN;
    p1->listeners.push_back(this);
    p1->setName("P");
    
    Pin* p2 = new Pin(Pin::Type::EVENT);
    p2->direction = Pin::Direction::IN;
    p2->listeners.push_back(this);
    p2->setName("G");
    
    Pin* p3 = new Pin(Pin::Type::VALUE);
    p3->direction = Pin::Direction::IN;
    p3->listeners.push_back(this);
    p3->setName("V");

    addPin(Pin::Direction::IN,p1);
    addPin(Pin::Direction::IN,p2);
    addPin(Pin::Direction::IN,p3);
}

void MidiOut::paint(juce::Graphics &g) {
    Module::paint(g);
    g.drawImageAt(ImageCache::getFromMemory(BinaryData::midiout_png, BinaryData::midiout_pngSize),25,40);
}

void MidiOut::message(juce::MidiMessage *message) {
    deviceManager->getDefaultMidiOutput()->sendMessageNow(*message);
}

void MidiOut::gate(bool on, int note, int velocity) {
    gateOn = on;
    this->velocity = velocity;
    
    if (on) {
        MidiMessage m = MidiMessage::noteOn(1, note, (uint8)velocity);
        Logger::writeToLog(m.getDescription());
        message(&m);
    }
    else {
        MidiMessage m2 = MidiMessage::noteOff(1, note, (uint8)0);
        Logger::writeToLog(m2.getDescription());
        message(&m2);
    }
    eventComplete = false;

}

void MidiOut::setNote(int value){
    this->note = value;
}

void MidiOut::setDeviceManager(juce::AudioDeviceManager* manager) {
    this->deviceManager = manager;
}

void MidiOut::eventReceived(Event *e) {
    
    if (e->getType() == Event::Type::CONTROLLER) {
        

        int value = e->getValue();

        deviceManager->getDefaultMidiOutput()->sendMessageNow(MidiMessage::controllerEvent(1, e->getNumber(), value));

        // float velocity = e->getValue()/127.0;
        //deviceManager->getDefaultMidiOutput()->sendMessageNow(MidiMessage::noteOn(1,e->getNote(), velocity));
        
    }
    
    /*
    if (raw == nullptr) {
        raw = new RawEvent();
    }
    
    if (e->getType() == Event::Type::NOTE) {
        note = e->getValue();
        if (raw->note == nullptr) {
            raw->note = e;
        }
    }
    else if (e->getType() == Event::Type::GATE) {
        if (raw->gate == nullptr) {
            raw->gate = e;
        }
    }
    
    if (raw->note != nullptr && raw->gate != nullptr) {
        if (raw->gate->getValue() > 0)
            gate(true,raw->note->getValue(), raw->gate->getValue());
        else
            gate(false,raw->note->getValue(), raw->gate->getValue());
        
        delete raw->gate;
        delete raw->note;
        
        raw->note = nullptr;
        raw->gate = nullptr;
        
        delete raw;
        raw = nullptr;
    }

    */
}
