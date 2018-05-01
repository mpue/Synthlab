/*
  ==============================================================================

    SawtoothModule
    Created: 4 Apr 2018 3:48:23pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "NoiseModule.h"
#include "Connection.h"
#include "AudioEngine/WhiteNoise.h"

//==============================================================================
NoiseModule::NoiseModule(double sampleRate, int buffersize)
{
    this->sampleRate = sampleRate;
    this->buffersize = buffersize;
    
    oscillator = new WhiteNoise(sampleRate, buffersize);
    
    oscillator->setVolume(0.5f);
    
    setSize(120,140);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(18,2);
    
    setName("White noise");

    
    editable = false;
    prefab = true;
}

NoiseModule::~NoiseModule()
{
    delete oscillator;
}

void NoiseModule::configurePins() {
    Pin* p1 = new Pin(Pin::Type::VALUE);
    p1->direction = Pin::Direction::IN;
    p1->listeners.push_back(this);
    p1->setName("A");
    
    Pin* p2 = new Pin(Pin::Type::AUDIO);
    p2->direction = Pin::Direction::OUT;
    p2->listeners.push_back(this);
    p2->setName("O");
    
    addPin(Pin::Direction::IN,p1);
    addPin(Pin::Direction::OUT,p2);
}

void NoiseModule::paint(juce::Graphics &g) {
    Module::paint(g);
    g.drawImageAt(ImageCache::getFromMemory(BinaryData::whitenoise_png, BinaryData::whitenoise_pngSize),25,40);
}



void NoiseModule::setAmplitude(float amplitude) {
    this->oscillator->setVolume(amplitude);
}

void NoiseModule::process() {
    
    if (pins.at(0)->connections.size() ==  1) {
        this->oscillator->setVolume(pins.at(0)->connections.at(0)->getValue());
    }
    for (int i = 0; i < buffersize; i++) {
        float value = oscillator->process();
        
        if (pins.at(1)->getAudioBuffer() != nullptr && pins.at(1)->getAudioBuffer()->getNumChannels() > 0)
            pins.at(1)->getAudioBuffer()->setSample(0,i ,value);

    }
    
}
