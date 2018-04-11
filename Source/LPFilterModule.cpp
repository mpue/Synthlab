/*
  ==============================================================================

    SawtoothModule
    Created: 4 Apr 2018 3:48:23pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "LPFilterModule.h"
#include "Connection.h"
#include "AudioEngine/LowPassFilter.h"

//==============================================================================
LPFilterModule::LPFilterModule(double sampleRate, int buffersize)
{
    this->sampleRate = sampleRate;
    this->buffersize = buffersize;
    
    filter = new LowPassFilter();
    
    setSize(120,140);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(18,2);
    
    setName("LPFilter");

    editable = false;
    prefab = true;
}

LPFilterModule::~LPFilterModule()
{

}

void LPFilterModule::configurePins() {
    Pin* p1 = new Pin(Pin::Type::VALUE);
    p1->direction = Pin::Direction::IN;
    p1->listeners.push_back(this);
    p1->setName("F");
    
    Pin* p2 = new Pin(Pin::Type::VALUE);
    p2->direction = Pin::Direction::IN;
    p2->listeners.push_back(this);
    p2->setName("R");
    
    Pin* p3 = new Pin(Pin::Type::AUDIO);
    p3->direction = Pin::Direction::IN;
    p3->listeners.push_back(this);
    p3->setName("In");
    
    Pin* p4 = new Pin(Pin::Type::AUDIO);
    p4->direction = Pin::Direction::OUT;
    p4->listeners.push_back(this);
    p4->setName("Out");
    
    addPin(Pin::Direction::IN,p1);
    addPin(Pin::Direction::IN,p2);
    addPin(Pin::Direction::IN,p3);
    addPin(Pin::Direction::OUT,p4);

}

void LPFilterModule::paint(juce::Graphics &g) {
    Module::paint(g);
    g.drawImageAt(ImageCache::getFromMemory(BinaryData::sawtooth_png, BinaryData::sawtooth_pngSize),25,40);
}


void LPFilterModule::process() {
    
    if (pins.at(0)->connections.size() ==  1) {
   
    }
    if (pins.at(1)->connections.size() ==  1) {
        // this->setFine(pins.at(1)->connections.at(0)->getValue());
    }
    if (pins.at(2)->connections.size() ==  1) {
        //this->setAmplitude(abs(pins.at(2)->connections.at(0)->getValue()));
    }
    for (int i = 0; i < buffersize; i++) {
        /*
        const float* in = pins.at(2)->getAudioBuffer()->getReadPointer(0);
        float* out = pins.at(3)->getAudioBuffer()->getWritePointer(0);
        
        float value = filter->process((const float*)in,out,1);
        
        if (pins.at(3)->getAudioBuffer() != nullptr && pins.at(3)->getAudioBuffer()->getNumChannels() > 0)
            pins.at(3)->getAudioBuffer()->setSample(0,i ,value);
         */
    }
    
}
