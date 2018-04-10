/*
  ==============================================================================

    MidiGate.cpp
    Created: 4 Apr 2018 3:48:23pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "AdderModule.h"
#include "Connection.h"


//==============================================================================
AdderModule::AdderModule()
{
    setSize(120,70);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(18,2);
    setName("Add");
    editable = false;
    prefab = true;
}

AdderModule::~AdderModule()
{   
}

void AdderModule::configurePins() {
    Pin* p1 = new Pin(Pin::Type::AUDIO);
    p1->direction = Pin::Direction::IN;
    p1->setName("A");
    
    Pin* p2 = new Pin(Pin::Type::AUDIO);
    p2->direction = Pin::Direction::IN;
    p2->setName("B");
    
    Pin* p3 = new Pin(Pin::Type::AUDIO);
    p3->direction = Pin::Direction::OUT;
    p3->setName("C");
    
    addPin(Pin::Direction::IN,p1);
    addPin(Pin::Direction::IN,p2);
    addPin(Pin::Direction::OUT,p3);
}

void AdderModule::paint(juce::Graphics &g) {
    Module::paint(g);
}

void AdderModule::process() {
    
    float* out = pins.at(2)->getAudioBuffer()->getWritePointer(0);

    if (pins.at(0)->connections.size() > 0 && pins.at(1)->connections.size() > 0) {

        AudioSampleBuffer* buffer1 = pins.at(0)->connections.at(0)->getAudioBuffer();
        AudioSampleBuffer* buffer2 = pins.at(1)->connections.at(0)->getAudioBuffer();
        
        if (buffer1 != nullptr && buffer2 != nullptr) {
            const float* in1 = pins.at(0)->connections.at(0)->getAudioBuffer()->getReadPointer(0);
            const float* in2 = pins.at(1)->connections.at(0)->getAudioBuffer()->getReadPointer(0);
            for (int i = 0; i  <512;i++) {
                out[i] = in1[i] + in2[i];
            }
            
        }
        else {
            pins.at(2)->getAudioBuffer()->clear();
        }
    }



}

