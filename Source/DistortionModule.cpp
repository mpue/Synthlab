/*
 ==============================================================================
 
 SawtoothModule
 Created: 4 Apr 2018 3:48:23pm
 Author:  Matthias Pueski
 
 ==============================================================================
 */

#include "../JuceLibraryCode/JuceHeader.h"
#include "DistortionModule.h"
#include "Connection.h"
#include "AudioEngine/Distortion.h"

using juce::Justification;

//==============================================================================
DistortionModule::DistortionModule(double sampleRate, int buffersize)
{
    this->sampleRate = sampleRate;
    this->buffersize = buffersize;
    
    distortion = new Distortion();
    
    setSize(120,140);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(18,2);
    
    setName("Distortion");
    
    editable = false;
    prefab = true;
    
    buffer = new float[buffersize];

}

DistortionModule::DistortionModule()
{
    delete buffer;
    delete distortion;
}

void DistortionModule::configurePins() {
    Pin* p1 = new Pin(Pin::Type::AUDIO);
    p1->direction = Pin::Direction::IN;
    p1->listeners.push_back(this);
    p1->setName("In");
    
    p1->setDescription("Audio input");
    
    Pin* p2 = new Pin(Pin::Type::AUDIO);
    p2->direction = Pin::Direction::OUT;
    p2->listeners.push_back(this);
    p2->setName("Out");
    
    p2->setDescription("Audio output");
    
    Pin* p3 = new Pin(Pin::Type::VALUE);
    p3->direction = Pin::Direction::IN;
    p3->listeners.push_back(this);
    p3->setName("Drive");
    
    p3->setDescription("Drive of the module. Typical range [0..5]");
   
    Pin* p4 = new Pin(Pin::Type::VALUE);
    p4->direction = Pin::Direction::IN;
    p4->listeners.push_back(this);
    p4->setName("Mix");
    
    p4->setDescription("Mix factor determines how much of the distorion is being sent to the output. Typical range [0..1]");
    
    Pin* p5 = new Pin(Pin::Type::VALUE);
    p5->direction = Pin::Direction::IN;
    p5->listeners.push_back(this);
    p5->setName("Type");
    
    p5->setDescription("Distortion type. Typical range [0..6]");
    
    addPin(Pin::Direction::IN,p1);
    addPin(Pin::Direction::OUT,p2);
    addPin(Pin::Direction::IN,p3);
    addPin(Pin::Direction::IN,p4);
    addPin(Pin::Direction::IN,p5);
}

void DistortionModule::paint(juce::Graphics &g) {
    Module::paint(g);
    // g.drawImageAt(ImageCache::getFromMemory(BinaryData::lp_png, BinaryData::lp_pngSize),25,40);
}


void DistortionModule::process() {
    
    if (pins.at(0)->getConnections().size() == 1 ) {
        const float* in = pins.at(0)->getConnections().at(0)->getAudioBuffer()->getReadPointer(0);
        
        for (int i = 0; i < buffersize;i++){
            buffer[i] = distortion->processSample(in[i]);
            pins.at(1)->getAudioBuffer()->setSample(0, i,buffer[i]);
        }
    }
    
    if (pins.at(2)->getConnections().size() ==  1) {
        distortion->controls.drive = pins.at(2)->getConnections().at(0)->getValue();
    }
    if (pins.at(3)->getConnections().size() ==  1) {
        distortion->controls.mix = pins.at(3)->getConnections().at(0)->getValue();
    }
    if (pins.at(4)->getConnections().size() ==  1) {
        distortion->controls.mode = pins.at(4)->getConnections().at(0)->getValue();
    }

}

