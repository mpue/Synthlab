/*
  ==============================================================================

    SamplerModule.cpp
    Created: 17 Apr 2018 3:34:36pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "SamplerModule.h"
#include "../JuceLibraryCode/JuceHeader.h"
#include "Connection.h"

//==============================================================================
SamplerModule::SamplerModule(double sampleRate, int buffersize, AudioFormatManager* manager)
{
    this->sampleRate = sampleRate;
    this->buffersize = buffersize;
    
    sampler = new Sampler(sampleRate, buffersize, manager);
    cache = new AudioThumbnailCache(1);
    thumbnail = new AudioThumbnail(buffersize   ,*manager,*cache);
    
    sampler->setVolume(0.5f);
    
    setSize(256,140);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(18,2);
    
    setName("Sampler");
    
    MemoryInputStream* mis = new MemoryInputStream(BinaryData::boom_aif,BinaryData::boom_aifSize, false);
    sampler->loadSample(mis);
    this->thumbnail->reset(2, sampleRate);
    thumbnail->addBlock(0, *sampler->getSampleBuffer(), 0, sampler->getSampleLength());


    
    editable = false;
    prefab = true;
}

SamplerModule::~SamplerModule()
{
    delete sampler;
    delete thumbnail;
}

void SamplerModule::configurePins() {
    Pin* p1 = new Pin(Pin::Type::VALUE);
    p1->direction = Pin::Direction::IN;
    p1->listeners.push_back(this);
    p1->setName("A");
    
    Pin* p2 = new Pin(Pin::Type::AUDIO);
    p2->direction = Pin::Direction::OUT;
    p2->listeners.push_back(this);
    p2->setName("L");
    
    Pin* p3 = new Pin(Pin::Type::AUDIO);
    p3->direction = Pin::Direction::OUT;
    p3->listeners.push_back(this);
    p3->setName("R");
    
    Pin* p4 = new Pin(Pin::Type::EVENT);
    p4->direction = Pin::Direction::IN;
    p4->listeners.push_back(this);
    p4->setName("E");
    
    
    addPin(Pin::Direction::IN,p1);
    addPin(Pin::Direction::OUT,p2);
    addPin(Pin::Direction::OUT,p3);
    addPin(Pin::Direction::IN,p4);
}

void SamplerModule::paint(juce::Graphics &g) {
    Module::paint(g);
    Rectangle<int> tb = Rectangle<int>(10,10,236,120);
    g.setColour(Colours::black);
    g.fillRect(tb);
    g.setColour(Colours::orange);
    this->thumbnail->drawChannels(g, tb,0,sampler->getSampleLength() / sampleRate,1);

}



void SamplerModule::setAmplitude(float amplitude) {
    this->sampler->setVolume(amplitude);
}

void SamplerModule::process() {
    
    if (!gate) {
        return;
    }
    
    if (currentEnvelope < 0)
    for (int j = 0; j < 128;j++) {
        if(pins.at(0)->connections.at(0)->dataEnabled[j]) {
            currentEnvelope = j;
            break;
        }
    }
        
    
    sampler->setVolume(pins.at(0)->connections.at(0)->data[currentEnvelope]);
    
    if (pins.at(1)->getAudioBuffer() != nullptr && pins.at(1)->getAudioBuffer()->getNumChannels() > 0){
        for (int j = 0; j < buffersize;j++) {
            
            float valueL = sampler->getSampleAt(0, currentSample);
            float valueR = sampler->getSampleAt(1, currentSample);
            
            pins.at(1)->getAudioBuffer()->setSample(0,j ,valueL);
            pins.at(2)->getAudioBuffer()->setSample(0,j ,valueR);
            
            if (currentSample < sampler->getSampleLength() - 1) {
                currentSample++;
            }
            else {
                currentSample = 0;
            }
        }
    }
    
}


void SamplerModule::eventReceived(Event *e) {
    
    if (e->getType() == Event::Type::GATE) {
        if (e->getValue() > 0) {
            currentSample = 0;
            gate = true;
            currentEnvelope = e->getNote();
        }        
    }
    
}
