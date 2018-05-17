/*
 ==============================================================================
 
 SawtoothModule
 Created: 4 Apr 2018 3:48:23pm
 Author:  Matthias Pueski
 
 ==============================================================================
 */

#include "../JuceLibraryCode/JuceHeader.h"
#include "ChorusModule.h"
#include "Connection.h"
#include "AudioEngine/FractionalDelayBuffer.h"

using juce::Justification;

//==============================================================================
ChorusModule::ChorusModule(double sampleRate, int buffersize)
{
    this->sampleRate = sampleRate;
    this->buffersize = buffersize;
    
    leftBuffer = new FractionalDelayBuffer();
    rightBuffer = new FractionalDelayBuffer();
    
    leftOsc = new Sine(sampleRate, buffersize);
    rightOsc = new Sine(sampleRate, buffersize);
    
    leftBuffer->setBufferSize((int)buffersize * 10);
    rightBuffer->setBufferSize((int)buffersize * 10);
    leftOsc->setFrequency(0.5);
    rightOsc->setFrequency(0.6);

    setSize(120,140);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(18,2);
    
    setName("Chorus");
    
    editable = false;
    prefab = true;

}

ChorusModule::ChorusModule()
{
    delete leftOsc;
    delete rightOsc;
    delete leftBuffer;
    delete rightBuffer;
}

void ChorusModule::configurePins() {
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
    
    addPin(Pin::Direction::IN,p1);
    addPin(Pin::Direction::IN,p2);
    addPin(Pin::Direction::OUT,p3);
    addPin(Pin::Direction::OUT,p4);
}

void ChorusModule::paint(juce::Graphics &g) {
    Module::paint(g);
    g.drawImageAt(ImageCache::getFromMemory(BinaryData::module_png, BinaryData::module_pngSize),25,40);
}


void ChorusModule::process() {
    
    if (pins.at(0)->getConnections().size() == 1 && pins.at(1)->getConnections().size() == 1) {
        
        const float* inL = pins.at(0)->getConnections().at(0)->getAudioBuffer()->getReadPointer(0);
        const float* inR = pins.at(1)->getConnections().at(0)->getAudioBuffer()->getReadPointer(0);

        for (int i = 0; i < buffersize; i++) {
            
            float leftMod = (leftOsc->getOutput()+1.01) * modulation* 100;
            float rightMod = (leftOsc->getOutput()+1.01) * modulation* 100;
            
            leftDelayTime = (delay * 200) + leftMod + .002;
            rightDelayTime = (delay * 220) + rightMod + .0015;
            
            float l_xn = inL[i];
            float r_xn = inR[i];
            
            float l_combined;
            float r_combined;

            float l_yn;
            float r_yn;
            
            l_yn = leftBuffer->getSample(leftDelayTime);
            r_yn = rightBuffer->getSample(rightDelayTime);
            
            l_combined = l_xn + r_yn * feedback;
            r_combined = r_xn + l_yn * feedback;
            
            
            leftBuffer->addSample(l_combined);
            rightBuffer->addSample(r_combined);
            
            pins.at(2)->getAudioBuffer()->getWritePointer(0)[i] = (l_xn * (1-mix) + l_yn * mix);
            pins.at(3)->getAudioBuffer()->getWritePointer(0)[i] = (r_xn * (1-mix) + r_yn * mix);
         
            leftOsc->process();
            rightOsc->process();
        }
        
    }
    
}

