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

//==============================================================================
RandomModule::RandomModule(double sampleRate, int buffersize)
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
    
    Pin* p1 = new Pin(Pin::Type::VALUE);
    p1->direction = Pin::Direction::IN;
    p1->listeners.push_back(this);
    p1->setName("F");
    
    Pin* p2 = new Pin(Pin::Type::VALUE);
    p2->direction = Pin::Direction::IN;
    p2->listeners.push_back(this);
    p2->setName("A");
    
    Pin* p3 = new Pin(Pin::Type::AUDIO);
    p3->direction = Pin::Direction::OUT;
    p3->listeners.push_back(this);
    p3->setName("Out");
    
    Pin* p4 = new Pin(Pin::Type::VALUE);
    p4->direction = Pin::Direction::OUT;
    p4->listeners.push_back(this);
    p4->setName("V");

    addPin(Pin::Direction::IN,p1);
    addPin(Pin::Direction::IN,p2);
    addPin(Pin::Direction::OUT,p3);
    addPin(Pin::Direction::OUT,p4);
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

        pins.at(3)->setValue(abs(value + 1));
    }
    
}
