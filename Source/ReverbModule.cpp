/*
 ==============================================================================
 
 SawtoothModule
 Created: 4 Apr 2018 3:48:23pm
 Author:  Matthias Pueski
 
 ==============================================================================
 */

#include "../JuceLibraryCode/JuceHeader.h"
#include "ReverbModule.h"
#include "Connection.h"
#include "AudioEngine/StereoReverb.h"

using juce::Justification;

//==============================================================================
ReverbModule::ReverbModule(double sampleRate, int buffersize)
{
    this->sampleRate = sampleRate;
    this->buffersize = buffersize;
    
    reverb = new Reverb();
    
    setSize(120,150);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(18,2);
    
    setName("Reverb");
    
    editable = false;
    prefab = true;
    
    bufferLeft = new float[buffersize];
    bufferRight = new float[buffersize];
    
   
    
    reverb->setParameters(params);
}

ReverbModule::~ReverbModule()
{
    delete bufferLeft;
    delete bufferRight;
    delete reverb;
}

void ReverbModule::configurePins() {
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
    p5->setName("Da");

    Pin* p6 = new Pin(Pin::Type::VALUE);
    p6->direction = Pin::Direction::IN;
    p6->listeners.push_back(this);
    p6->setName("Wet");
    
    Pin* p7 = new Pin(Pin::Type::VALUE);
    p7->direction = Pin::Direction::IN;
    p7->listeners.push_back(this);
    p7->setName("Dry");
    
    Pin* p8 = new Pin(Pin::Type::VALUE);
    p8->direction = Pin::Direction::IN;
    p8->listeners.push_back(this);
    p8->setName("Si");
    
    Pin* p9 = new Pin(Pin::Type::VALUE);
    p9->direction = Pin::Direction::IN;
    p9->listeners.push_back(this);
    p9->setName("Fb");
    
    addPin(Pin::Direction::IN,p1);
    addPin(Pin::Direction::IN,p2);
    addPin(Pin::Direction::OUT,p3);
    addPin(Pin::Direction::OUT,p4);
    addPin(Pin::Direction::IN,p5);
    addPin(Pin::Direction::IN,p6);
    addPin(Pin::Direction::IN,p7);
    addPin(Pin::Direction::IN,p8);
    addPin(Pin::Direction::IN,p9);

}

void ReverbModule::paint(juce::Graphics &g) {
    Module::paint(g);
    // g.drawImageAt(ImageCache::getFromMemory(BinaryData::lp_png, BinaryData::lp_pngSize),25,40);
}


void ReverbModule::process() {
    
    if (pins.at(4)->getConnections().size() ==  1) {
        if (getDamping() != pins.at(4)->getConnections().at(0)->getValue()) {
            damping = pins.at(4)->getConnections().at(0)->getValue();
            setDamping(damping);
            reverb->setParameters(params);
        }
    }
    if (pins.at(5)->getConnections().size() ==  1) {
        if (getWetLevel() != pins.at(5)->getConnections().at(0)->getValue()) {
            wetLevel = (pins.at(5)->getConnections().at(0)->getValue());
            wetLevel = std::max(0.0f, std::min(wetLevel, 1.0f));
            setWetLevel(wetLevel);
            reverb->setParameters(params);
        }
    }
    if (pins.at(6)->getConnections().size() ==  1) {
        if (getDryLevel() != pins.at(6)->getConnections().at(0)->getValue()) {
            dryLevel = (pins.at(5)->getConnections().at(0)->getValue());
            dryLevel = std::max(0.0f, std::min(dryLevel, 1.0f));
            setDryLevel(dryLevel);
            reverb->setParameters(params);
        }
    }
    if (pins.at(7)->getConnections().size() ==  1) {
        if (getRoomSize() != pins.at(7)->getConnections().at(0)->getValue()) {
            roomSize = pins.at(7)->getConnections().at(0)->getValue();
            roomSize = std::max(0.0f, std::min(roomSize, 1.0f));
            setRoomSize(roomSize);
            reverb->setParameters(params);
        }
    }
    if (pins.at(8)->getConnections().size() ==  1) {
        if (getFeedbackLevel() != pins.at(8)->getConnections().at(0)->getValue()) {
            freezeMode = pins.at(8)->getConnections().at(0)->getValue();
            freezeMode = std::max(0.0f, std::min(freezeMode, 1.0f));
            setFeedbackLevel(freezeMode);
            reverb->setParameters(params);
        }
    }
    
    if (pins.at(0)->getConnections().size() == 1 && pins.at(1)->getConnections().size() == 1) {
        const float* inL = pins.at(0)->getConnections().at(0)->getAudioBuffer()->getReadPointer(0);
        const float* inR = pins.at(1)->getConnections().at(0)->getAudioBuffer()->getReadPointer(0);
        
        for (int i = 0; i < buffersize;i++){
            bufferLeft[i] = inL[i];
            bufferRight[i] = inR[i];
        }
        
        reverb->processStereo(bufferLeft, bufferRight, buffersize);
        
        pins.at(2)->getAudioBuffer()->copyFrom(0, 0, bufferLeft, buffersize);
        pins.at(3)->getAudioBuffer()->copyFrom(0, 0, bufferRight, buffersize);
        
    }
    else {
        for (int i = 0; i < buffersize;i++){
            bufferLeft[i] = 0;
            bufferRight[i] = 0;
        }
        
        pins.at(2)->getAudioBuffer()->copyFrom(0, 0, bufferLeft, buffersize);
        pins.at(3)->getAudioBuffer()->copyFrom(0, 0, bufferRight, buffersize);
    }
    
}


void ReverbModule::setDamping(float damping) {
    this->params.damping = damping;
}

float ReverbModule::getDamping() {
    return this->params.damping;
}

void ReverbModule::setRoomSize(float size) {
    this->params.roomSize = size;
}

float ReverbModule::getRoomSize(){
    return this->params.roomSize;
}

void ReverbModule::setWetLevel(float level){
    this->params.wetLevel = level;
}

float ReverbModule::getWetLevel() {
    return this->params.wetLevel;
}

void ReverbModule::setDryLevel(float level) {
     this->params.dryLevel = level;
}

float ReverbModule::getDryLevel() {
    return params.dryLevel;
}

void ReverbModule::setFeedbackLevel(float level) {
    this->params.freezeMode = level;
}

float ReverbModule::getFeedbackLevel(){
    return this->params.freezeMode;
}

