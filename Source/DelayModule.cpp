/*
  ==============================================================================

    SawtoothModule
    Created: 4 Apr 2018 3:48:23pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "DelayModule.h"
#include "Connection.h"
#include "AudioEngine/LowPassFilter.h"

//==============================================================================
DelayModule::DelayModule(double sampleRate, int buffersize)
{
    this->sampleRate = sampleRate;
    this->buffersize = buffersize;
    
    delay = new StereoDelay();
    delay->setEnabled(true);
    
    setSize(120,140);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(18,2);
    
    setName("Stereo delay");

    editable = false;
    prefab = true;
    
    bufferLeft = new float[buffersize];
    bufferRight = new float[buffersize];
}

DelayModule::~DelayModule()
{
    delete bufferLeft;
    delete bufferRight;
    delete delay;
}

void DelayModule::configurePins() {
    Pin* p1 = new Pin(Pin::Type::AUDIO);
    p1->direction = Pin::Direction::IN;
    p1->listeners.push_back(this);
    p1->setName("L");
    
    Pin* p2 = new Pin(Pin::Type::AUDIO);
    p2->direction = Pin::Direction::IN;
    p2->listeners.push_back(this);
    p2->setName("R");
    
    
    Pin* p3 = new Pin(Pin::Type::AUDIO);
    p3->direction = Pin::Direction::OUT;
    p3->listeners.push_back(this);
    p3->setName("L");
    
    Pin* p4 = new Pin(Pin::Type::AUDIO);
    p4->direction = Pin::Direction::OUT;
    p4->listeners.push_back(this);
    p4->setName("R");
    
    Pin* p5 = new Pin(Pin::Type::VALUE);
    p5->direction = Pin::Direction::IN;
    p5->listeners.push_back(this);
    p5->setName("TL");
    
    Pin* p6 = new Pin(Pin::Type::VALUE);
    p6->direction = Pin::Direction::IN;
    p6->listeners.push_back(this);
    p6->setName("TR");
   
    Pin* p7 = new Pin(Pin::Type::VALUE);
    p7->direction = Pin::Direction::IN;
    p7->listeners.push_back(this);
    p7->setName("Fb");
    
    Pin* p8 = new Pin(Pin::Type::VALUE);
    p8->direction = Pin::Direction::IN;
    p8->listeners.push_back(this);
    p8->setName("Mix");
    
    addPin(Pin::Direction::IN,p1);
    addPin(Pin::Direction::IN,p2);
    addPin(Pin::Direction::OUT,p3);
    addPin(Pin::Direction::OUT,p4);
    addPin(Pin::Direction::IN,p5);
    addPin(Pin::Direction::IN,p6);
    addPin(Pin::Direction::IN,p7);
    addPin(Pin::Direction::IN,p8);
}

void DelayModule::paint(juce::Graphics &g) {
    Module::paint(g);
    // g.drawImageAt(ImageCache::getFromMemory(BinaryData::lp_png, BinaryData::lp_pngSize),25,40);
}


void DelayModule::process() {
    
    if (pins.at(0)->connections.size() == 1 && pins.at(1)->connections.size() == 1) {
        const float* inL = pins.at(0)->connections.at(0)->getAudioBuffer()->getReadPointer(0);
        const float* inR = pins.at(1)->connections.at(0)->getAudioBuffer()->getReadPointer(0);
        
        for (int i = 0; i < buffersize;i++){
            bufferLeft[i] = inL[i];
            bufferRight[i] = inR[i];
        }
        
        delay->processStereo(bufferLeft, bufferRight, buffersize);
        
        for (int i = 0; i < buffersize;i++){
            pins.at(2)->getAudioBuffer()->setSample(0, i,bufferLeft[i]);
            pins.at(3)->getAudioBuffer()->setSample(0, i,bufferRight[i]);
        }
        
    }
    if (pins.at(4)->connections.size() ==  1) {
        if (delayL != pins.at(4)->connections.at(0)->getValue()) {
            delay->setDelay(StereoDelay::Channel::LEFT,pins.at(4)->connections.at(0)->getValue());
        }
        
    }
    if (pins.at(5)->connections.size() ==  1) {
        if (delayR != pins.at(5)->connections.at(0)->getValue()) {
            delay->setDelay(StereoDelay::Channel::RIGHT,pins.at(5)->connections.at(0)->getValue());
        }
    }
    if (pins.at(6)->connections.size() ==  1) {
        if (feedback != pins.at(6)->connections.at(0)->getValue()) {
            delay->setFeedback(StereoDelay::Channel::LEFT,pins.at(6)->connections.at(0)->getValue());
            delay->setFeedback(StereoDelay::Channel::RIGHT,pins.at(6)->connections.at(0)->getValue());
        }
    }
    if (pins.at(7)->connections.size() ==  1) {
        if (mix != pins.at(7)->connections.at(0)->getValue()) {
            delay->setMix(StereoDelay::Channel::LEFT,pins.at(7)->connections.at(0)->getValue());
            delay->setMix(StereoDelay::Channel::RIGHT,pins.at(7)->connections.at(0)->getValue());
        }
    }
    
}
