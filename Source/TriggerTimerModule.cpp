/*
 ==============================================================================
 
 TriggerTimerModule.cpp
 Created: 4 Apr 2018 3:48:23pm
 Author:  Matthias Pueski
 
 ==============================================================================
 */

#include "../JuceLibraryCode/JuceHeader.h"
#include "TriggerTimerModule.h"
#include "Connection.h"
#include "MidiOut.h"

using juce::Justification;
using juce::ImageCache;
using juce::Value;
using juce::PropertyComponent;
using juce::SliderPropertyComponent;
using juce::Array;

//==============================================================================
TriggerTimerModule::TriggerTimerModule()
{
    setSize(120,140);
    
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(2,2);
    editable = false;
    prefab = true;
    
    createProperties();
    //startTimer(500);
}

TriggerTimerModule::~TriggerTimerModule()
{
    delete channelValue;
    delete channelListener;
    // stopTimer();
    // delete channelProp;
}

void TriggerTimerModule::configurePins() {
    
    gatePin = new Pin(Pin::Type::EVENT);
    gatePin->direction = Pin::Direction::IN;
    gatePin->listeners.push_back(this);
    gatePin->setDescription("Gate event in (use ADSR or Gate module for triggering.");
    gatePin->setName("G");
    addPin(Pin::Direction::IN,gatePin);
    
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

void TriggerTimerModule::paint(juce::Graphics &g) {
    Module::paint(g);
    g.drawImageAt(ImageCache::getFromMemory(BinaryData::clock_png, BinaryData::clock_pngSize),25,40);
}

void TriggerTimerModule::timerCallback() {
    
    if (currentTrigger < numTriggers) {
        currentTrigger++;
        Event* e = new Event("gate on",Event::Type::GATE);
        
        e->setNote(juce::Random().nextInt(127));
        e->setValue(64);
        
        
        for (int i = 0; i < pins.at(2)->getConnections().size();i++) {
            pins.at(2)->getConnections().at(i)->sendEvent(new Event(e));
        }
        
        delete e;
    }
    
    else {
        currentTrigger = 0 ;
        stopTimer();
    }
}

void TriggerTimerModule::eventReceived(Event *eventIn)  {
    
    if (currentTrigger == 0) {
        startTimer(250);
    }
}


void TriggerTimerModule::trigger(int interval) {
    startTimer(interval);
}

void TriggerTimerModule::createProperties() {
    channelValue = new Value();
    channelListener = new ChannelListener(*channelValue, this);
}

juce::Array<PropertyComponent*>& TriggerTimerModule::getProperties() {
    
    properties = juce::Array<PropertyComponent*>();
    
    channelProp = new SliderPropertyComponent(*channelValue,"channel",1,16,1,1,true);
    properties.add(channelProp);
    return properties;
}

int TriggerTimerModule::getChannel() {
    return channel;
}

void TriggerTimerModule::setChannel(int channel) {
    if (channel < 1) {
        channel = 1;
    }
    this->channel = channel;
    this->channelValue->setValue(channel);
}

