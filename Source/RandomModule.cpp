/*
  ==============================================================================

    SawtoothModule
    Created: 4 Apr 2018 3:48:23pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "RandomModule.h"
#include "Connection.h"
#include "AudioEngine/Sine.h"

using juce::Justification;
using juce::ImageCache;

//==============================================================================
RandomModule::RandomModule(double sampleRate, int buffersize) :
    gatePin(nullptr), p1(nullptr), p2(nullptr), p3(nullptr), p4(nullptr), eventOutPin(nullptr)
{
    this->sampleRate = sampleRate;
    this->buffersize = buffersize;
    
    oscillator = new Synthlab::Random(sampleRate, buffersize);
    
    oscillator->setFrequency(440);
    oscillator->setVolume(0.5f);
    
    setSize(120,140);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(18,2);
    
    setName("Random");

    editable = false;
    prefab = true;
}

RandomModule::~RandomModule()
{
    delete oscillator;
}

void RandomModule::setAmplitude(float amplitude) {
    this->oscillator->setVolume(amplitude);
}

void RandomModule::configurePins() {
    
    gatePin = new Pin(Pin::Type::EVENT);
    gatePin->direction = Pin::Direction::IN;
    gatePin->listeners.push_back(this);
    gatePin->setDescription("Gate event in (use ADSR or Gate module for triggering.");
    gatePin->setName("G");
    
    p1 = new Pin(Pin::Type::VALUE);
    p1->direction = Pin::Direction::IN;
    p1->listeners.push_back(this);
    p1->setName("F");
    
    p2 = new Pin(Pin::Type::VALUE);
    p2->direction = Pin::Direction::IN;
    p2->listeners.push_back(this);
    p2->setName("A");
    
    p3 = new Pin(Pin::Type::AUDIO);
    p3->direction = Pin::Direction::OUT;
    p3->listeners.push_back(this);
    p3->setName("Out");
    
    p4 = new Pin(Pin::Type::VALUE);
    p4->direction = Pin::Direction::OUT;
    p4->listeners.push_back(this);
    p4->setName("V");
    
    eventOutPin = new Pin(Pin::Type::EVENT);
    eventOutPin->direction = Pin::Direction::OUT;
    eventOutPin->listeners.push_back(this);
    eventOutPin->setDescription("Produces random node values on each trigger. Typical range [0..127].");
    eventOutPin->setName("R");

    addPin(Pin::Direction::IN,gatePin);
    addPin(Pin::Direction::IN,p1);
    addPin(Pin::Direction::IN,p2);
    addPin(Pin::Direction::OUT,p3);
    addPin(Pin::Direction::OUT,p4);
    addPin(Pin::Direction::OUT,eventOutPin);
}

void RandomModule::paint(juce::Graphics &g) {
    Module::paint(g);
    g.drawImageAt(ImageCache::getFromMemory(BinaryData::random_png, BinaryData::random_pngSize),25,40);
}


void RandomModule::process() {
    
    if (pins.at(0)->getConnections().size() ==  1) {
        this->oscillator->setFrequency(pins.at(0)->getConnections().at(0)->getValue());
    }
    if (pins.at(1)->getConnections().size() ==  1) {
        this->setAmplitude(abs(pins.at(1)->getConnections().at(0)->getValue()));
    }
    for (int i = 0; i < buffersize; i++) {
        float value = oscillator->process();
        
        if (pins.at(2)->getAudioBuffer() != nullptr && pins.at(2)->getAudioBuffer()->getNumChannels() > 0)
            pins.at(2)->getAudioBuffer()->setSample(0,i ,value);

        pins.at(4)->setValue(abs(value + 1));
    }
    
}

void RandomModule::eventReceived(Event *eventIn) {
    
    if (eventIn->getType() == Event::Type::GATE) {
        
        Event* eventOut;
        int velocity = 0;
        int note = 0;
        
        if (eventIn->getValue() > 0) {
            eventOut = new Event("gate on",Event::Type::GATE);
            velocity = 64; // TODO add property
            note = random.nextInt(127); // TODO add property for min/max value
        }
        else {
            eventOut = new Event("gate off",Event::Type::GATE);
            velocity = 0;
            note = 0;
        }
        
        eventOut->setValue(velocity);
        eventOut->setNote(note);
        
        for (int i = 0; i < eventOutPin->getConnections().size();i++) {
            eventOutPin->getConnections().at(i)->sendEvent(new Event(eventOut));
        }
        
        delete eventOut;
        
    }
}
