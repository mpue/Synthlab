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

using juce::Justification;
using juce::ImageCache;
using juce::Value;
using juce::PropertyComponent;
using juce::SliderPropertyComponent;
using juce::Array;

//==============================================================================
MidiGate::MidiGate()
{   
    setSize(120,140);
    
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(2,2);
    editable = false;
    prefab = true;
    
    createProperties();
}

MidiGate::~MidiGate()
{
    delete channelValue;
    delete channelListener;
    // delete channelProp;
}

void MidiGate::configurePins() {

    
    Pin* p1 = new Pin(Pin::Type::VALUE);
    p1->direction = Pin::Direction::OUT;
    p1->listeners.push_back(this);
    p1->setName("G");
    p1->setDescription("Gate value out. Contains velocity and note value as well.");
    addPin(Pin::Direction::OUT,p1);
    
    Pin* p2 = new Pin(Pin::Type::EVENT);
    p2->direction = Pin::Direction::OUT;
    p2->listeners.push_back(this);
    p2->setName("E");
    p2->setDescription("Event out. Contains the velocity and note values of all 128 notes as well.");
    addPin(Pin::Direction::OUT,p2);
}

void MidiGate::paint(juce::Graphics &g) {
    Module::paint(g);
    g.drawImageAt(ImageCache::getFromMemory(BinaryData::midiin_png, BinaryData::midiin_pngSize),25,40);
}

void MidiGate::gateOn(int velocity, int note) {
    
    pins.at(0)->setValue(velocity);
    pins.at(0)->setNote(note);
    
    Event* e = new Event("gate on",Event::Type::GATE);
    e->setValue(velocity);
    e->setNote(note);
    
    for (int i = 0; i < pins.at(1)->getConnections().size();i++) {
        pins.at(1)->getConnections().at(i)->sendEvent(new Event(e));
    }
    
    delete e;
}

void MidiGate::gateOff(int note) {
    pins.at(0)->setValue(0);
    pins.at(0)->setNote(note);
    Event* e = new Event("gate off",Event::Type::GATE);
    e->setValue(0);
    e->setNote(note);
    
    for (int i = 0; i < pins.at(1)->getConnections().size();i++) {
        pins.at(1)->getConnections().at(i)->sendEvent(new Event(e));
    }
    
    delete e;
}

void MidiGate::createProperties() {
    channelValue = new Value();
    channelListener = new ChannelListener(*channelValue, this);
}

juce::Array<PropertyComponent*>& MidiGate::getProperties() {
    
    properties = juce::Array<PropertyComponent*>();
    
    channelProp = new SliderPropertyComponent(*channelValue,"channel",1,16,1,1,true);
    properties.add(channelProp);
    return properties;
}

int MidiGate::getChannel() {
    return channel;
}

void MidiGate::setChannel(int channel) {
    if (channel < 1) {
        channel = 1;
    }
    this->channel = channel;
    this->channelValue->setValue(channel);
}

