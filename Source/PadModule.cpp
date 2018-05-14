/*
  ==============================================================================

    PadModule.cpp
    Created: 9 May 2018 11:49:06pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "PadModule.h"
#include "Connection.h"
#include "MidiOut.h"

using juce::Justification;
using juce::ImageCache;
using juce::Value;
using juce::PropertyComponent;
using juce::SliderPropertyComponent;
using juce::Array;

//==============================================================================
PadModule::PadModule()
{
    setSize(140,140);
    
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(2,2);
    editable = false;
    prefab = true;
    button = new juce::TextButton("Pad");
    button->setBounds(20, 20, getWidth()-40, getHeight()-40);
    button->addListener(this);
    addAndMakeVisible(button);
    setInterceptsMouseClicks(false,true);

    createProperties();
}

PadModule::~PadModule()
{
    delete channelValue;
    delete channelListener;
    delete noteValue;
    delete noteListener;
    delete holdValue;
    delete holdListener;
    delete button;
}

void PadModule::configurePins() {
    
    
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

void PadModule::paint(juce::Graphics &g) {
    
    if (currentLayer == Layer::ALL) {
        Module::paint(g);
    }
    else {
        Component::paint(g);
    }
    if (selected) {
        g.setColour(Colours::orange);
        g.drawRect(0,0,getWidth(),getHeight());
    }
}

void PadModule::gateOn(int velocity, int note) {
    
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

void PadModule::gateOff(int note) {
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

void PadModule::createProperties() {
    channelValue = new Value();
    channelListener = new ChannelListener(*channelValue, this);
    noteValue = new Value();
    noteListener = new NoteListener(*noteValue, this);
    holdValue = new Value();
    holdListener = new HoldListener(*holdValue, this);
}

juce::Array<PropertyComponent*>& PadModule::getProperties() {
    
    properties = juce::Array<PropertyComponent*>();
    
    channelProp = new SliderPropertyComponent(*channelValue,"channel",1,16,1,1,true);
    properties.add(channelProp);
    
    noteProp = new SliderPropertyComponent(*noteValue,"note",0,127,1,1,true);
    properties.add(noteProp);
    
    holdProp = new BooleanPropertyComponent(*holdValue,"Hold","Hold");
    properties.add(holdProp);
    
    return properties;
}

int PadModule::getChannel() {
    return channel;
}

void PadModule::setChannel(int channel) {
    if (channel < 1) {
        channel = 1;
    }
    this->channel = channel;
    this->channelValue->setValue(channel);
}

int PadModule::getNote() {
    return note;
}

void PadModule::setNote(int note) {
    this->note = note;
    this->noteValue->setValue(note);
}

bool PadModule::isHold() {
    return holdMode;
}

void PadModule::setHold(bool hold){
    this->holdMode = hold;
}

void PadModule::buttonClicked (Button*)
{
}

void PadModule::buttonStateChanged (Button* button)
{
    if (isHold()) {
        
        if (juce::Button::ButtonState::buttonDown == button->getState()) {
            buttonDown = !buttonDown;
            
            if(buttonDown)
            {
                gateOn(64, note);
            }
            else {
                gateOff(note);
            }
        }
    }
    else {
        if(juce::Button::ButtonState::buttonDown == button->getState())
        {
            gateOn(64, note);
        }
        else {
            gateOff(note);
        }
    }
}

