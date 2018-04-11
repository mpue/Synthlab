/*
  ==============================================================================

    SawtoothModule
    Created: 4 Apr 2018 3:48:23pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "ADSRModule.h"
#include "Connection.h"
#include "AudioEngine/Sawtooth.h"

//==============================================================================
ADSRModule::ADSRModule(double sampleRate, int buffersize)
{
    this->sampleRate = sampleRate;
    this->buffersize = buffersize;

    setSize(120,140);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(18,2);
    
    setName("ADSR");

    this->envelope = new ADSR();
    
    editable = false;
    prefab = true;
}

ADSRModule::~ADSRModule()
{

}

void ADSRModule::configurePins() {
    
    Pin* p1 = new Pin(Pin::Type::VALUE);
    p1->direction = Pin::Direction::IN;
    p1->listeners.push_back(this);
    p1->setName("G");
    
    Pin* p2 = new Pin(Pin::Type::VALUE);
    p2->direction = Pin::Direction::IN;
    p2->listeners.push_back(this);
    p2->setName("A");
    
    Pin* p3 = new Pin(Pin::Type::VALUE);
    p3->direction = Pin::Direction::IN;
    p3->listeners.push_back(this);
    p3->setName("D");
    
    Pin* p4 = new Pin(Pin::Type::VALUE);
    p4->direction = Pin::Direction::IN;
    p4->listeners.push_back(this);
    p4->setName("S");
    
    Pin* p5 = new Pin(Pin::Type::VALUE);
    p5->direction = Pin::Direction::IN;
    p5->listeners.push_back(this);
    p5->setName("R");
    
    Pin* p6= new Pin(Pin::Type::VALUE);
    p6->direction = Pin::Direction::OUT;
    p6->listeners.push_back(this);
    p6->setName("Out");
    
    addPin(Pin::Direction::IN,p1);
    addPin(Pin::Direction::IN,p2);
    addPin(Pin::Direction::IN,p3);
    addPin(Pin::Direction::IN,p4);
    addPin(Pin::Direction::IN,p5);
    addPin(Pin::Direction::OUT,p6);
}

void ADSRModule::paint(juce::Graphics &g) {
    Module::paint(g);
    g.drawImageAt(ImageCache::getFromMemory(BinaryData::sawtooth_png, BinaryData::sawtooth_pngSize),25,40);
}

void ADSRModule::setAttack(float attack) {
    this->attack = attack;
    this->envelope->setAttackRate(sampleRate * attack);
}

void ADSRModule::setDecay(float decay) {
    this->decay = decay;
    this->envelope->setDecayRate(sampleRate * decay);
}

void ADSRModule::setSustain(float sustain) {
    this->sustain = sustain;
    this->envelope->setSustainLevel(sustain);
}

void ADSRModule::setRelease(float release) {
    this->release = release;
    this->envelope->setReleaseRate(sampleRate * release);
}

void ADSRModule::process() {
    
    if (pins.at(0)->connections.size() ==  1) {
        if (this->value != pins.at(0)->connections.at(0)->getValue()) {
            
            if (this->value < pins.at(0)->connections.at(0)->getValue()) {
                this->gate = true;
                this->envelope->gate(pins.at(0)->connections.at(0)->getValue());
            }
            else {
                this->gate = false;
                this->envelope->gate(0);
            }
            
            this->value = pins.at(0)->connections.at(0)->getValue();
        }

    }
    if (pins.at(1)->connections.size() ==  1) {
        this->setAttack(abs(pins.at(1)->connections.at(0)->getValue()));
    }
    if (pins.at(2)->connections.size() ==  1) {
        this->setDecay(abs(pins.at(2)->connections.at(0)->getValue()));
    }
    
    if (pins.at(3)->connections.size() ==  1) {
        this->setSustain(abs(pins.at(3)->connections.at(0)->getValue()));
    }
    
    if (pins.at(4)->connections.size() ==  1) {
        this->setRelease(abs(pins.at(4)->connections.at(0)->getValue()));
    }
  
    float value = envelope->process();
    pins.at(5)->setValue(value);
    
}
