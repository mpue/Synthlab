/*
 ==============================================================================
 
 KeyboardModule
 Created: 4 Apr 2018 3:48:23pm
 Author:  Matthias Pueski
 
 ==============================================================================
 */

#include "../JuceLibraryCode/JuceHeader.h"
#include "KeyboardModule.h"
#include "FileButtonPropertyComponent.h"

using juce::Justification;
using juce::Value;
using juce::PropertyComponent;
using juce::SliderPropertyComponent;
using juce::Array;
using juce::String;

//==============================================================================
KeyboardModule::KeyboardModule()
{
    
    setSize(340,100);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(2,2);
    editable = false;
    prefab = true;
    keyboard = new MidiKeyboardComponent(state, orientation);
    
    keyboard->setEnabled(false);
    keyboard->setKeyWidth(50);
    addAndMakeVisible(keyboard);
    createProperties();
    resizer = new ResizableCornerComponent(this,nullptr);
    resizer->setSize(getWidth(), getHeight());
    addAndMakeVisible(resizer);
    resizer->toFront(true);
    resizer->setTopLeftPosition(getWidth()-10, getHeight()-10);
    resizer->setSize(10,10);
    setInterceptsMouseClicks(false, true);
    // addMouseListener(resizer, true);
    //addMouseListener(resizer, true);
    state.addListener(this);
    resized();
}

KeyboardModule::~KeyboardModule()
{
    delete channelValue;
    delete channelListener;
    delete resizer;
    delete keyboard;
    state.removeListener(this);
}

void KeyboardModule::configurePins() {
    
    
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

void KeyboardModule::gateOn(int velocity, int note) {
    
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

void KeyboardModule::gateOff(int note) {
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

void KeyboardModule::setCurrentLayer(Module::Layer layer) {
    Module::setCurrentLayer(layer);
    
    if (currentLayer == Module::Layer::ALL) {
        setInterceptsMouseClicks(false,false);
    }
    else {
        setInterceptsMouseClicks(false,false);
    }
}

void KeyboardModule::paint(juce::Graphics &g) {
    Module::paint(g);
    if (selected) {
        g.setColour(Colours::orange);
        g.drawRect(0,0,getWidth(),getHeight());
    }
}

void KeyboardModule::resized() {
    if (resizer != nullptr) {
        resizer->setTopLeftPosition(getWidth() - 10, getHeight() - 10);
    }
    if (keyboard != nullptr) {
        keyboard->setSize(getWidth() - 15, getHeight());
    }
    
    if (pins.size() == 2) {
        if (pins.at(0) != nullptr && pins.at(1) != nullptr) {
            pins.at(0)->x = getWidth() - 10;
            pins.at(1)->x = getWidth() - 10;
        }
    }
}

void KeyboardModule::createProperties() {
    channelValue = new Value();
    channelListener = new ChannelListener(*channelValue, this);
}

juce::Array<PropertyComponent*>& KeyboardModule::getProperties() {
    
    properties = juce::Array<PropertyComponent*>();
    
    channelProp = new SliderPropertyComponent(*channelValue,"channel",1,16,1,1,true);
    properties.add(channelProp);
    
    return properties;
}

int KeyboardModule::getChannel() {
    return channel;
}

void KeyboardModule::setChannel(int channel) {
    if (channel < 1) {
        channel = 1;
    }
    this->channel = channel;
    this->channelValue->setValue(channel);
}

void KeyboardModule::handleNoteOn (MidiKeyboardState* source,int midiChannel, int midiNoteNumber, float velocity) {
    gateOn(127 * velocity, midiNoteNumber);
}

void KeyboardModule::handleNoteOff (MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity) {
    gateOff(midiNoteNumber);
}

