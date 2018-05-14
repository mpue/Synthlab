/*
 ==============================================================================
 
 MidiClock.cpp
 Created: 4 Apr 2018 3:48:23pm
 Author:  Matthias Pueski
 
 ==============================================================================
 */

#include "../JuceLibraryCode/JuceHeader.h"
#include "MidiClock.h"
#include "Connection.h"
#include "MidiOut.h"

using juce::Justification;
using juce::ImageCache;
using juce::Value;
using juce::PropertyComponent;
using juce::SliderPropertyComponent;
using juce::Array;

//==============================================================================
MidiClock::MidiClock()
{
    setSize(120,140);
    
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(2,2);
    editable = false;
    prefab = true;
    
    createProperties();
    //startTimer(500);
}

MidiClock::~MidiClock()
{
    delete channelValue;
    delete channelListener;
    // stopTimer();
    // delete channelProp;
}

void MidiClock::configurePins() {
    
    
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

void MidiClock::paint(juce::Graphics &g) {
    Module::paint(g);
    g.drawImageAt(ImageCache::getFromMemory(BinaryData::clock_png, BinaryData::clock_pngSize),25,40);
}

void MidiClock::timerCallback() {
        
    Event* e = new Event("clock",Event::Type::CLOCK);
    
    for (int i = 0; i < pins.at(1)->getConnections().size();i++) {
        pins.at(1)->getConnections().at(i)->sendEvent(new Event(e));
    }
    
    delete e;
}

void MidiClock::clock(juce::MidiMessage message) {
    Event* e = new Event("clock",Event::Type::CLOCK);
    
    for (int i = 0; i < pins.at(1)->getConnections().size();i++) {
        pins.at(1)->getConnections().at(i)->sendEvent(new Event(e));
    }
    
    delete e;
}

void MidiClock::midiStart() {
    Event* e = new Event("clock",Event::Type::CLOCK_START);
    
    for (int i = 0; i < pins.at(1)->getConnections().size();i++) {
        pins.at(1)->getConnections().at(i)->sendEvent(new Event(e));
    }
    
    delete e;
}

void MidiClock::midiStop() {
    Event* e = new Event("clock",Event::Type::CLOCK_STOP);
    
    for (int i = 0; i < pins.at(1)->getConnections().size();i++) {
        pins.at(1)->getConnections().at(i)->sendEvent(new Event(e));
    }
    
    delete e;
}



void MidiClock::createProperties() {
    channelValue = new Value();
    channelListener = new ChannelListener(*channelValue, this);
}

juce::Array<PropertyComponent*>& MidiClock::getProperties() {
    
    properties = juce::Array<PropertyComponent*>();
    
    channelProp = new SliderPropertyComponent(*channelValue,"channel",1,16,1,1,true);
    properties.add(channelProp);
    return properties;
}

int MidiClock::getChannel() {
    return channel;
}

void MidiClock::setChannel(int channel) {
    if (channel < 1) {
        channel = 1;
    }
    this->channel = channel;
    this->channelValue->setValue(channel);
}

