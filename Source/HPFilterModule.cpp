/*
  ==============================================================================

    SawtoothModule
    Created: 4 Apr 2018 3:48:23pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "HPFilterModule.h"
#include "Connection.h"
#include "AudioEngine/HighPassFilter.h"

//==============================================================================
HPFilterModule::HPFilterModule(double sampleRate, int buffersize)
{
    this->sampleRate = sampleRate;
    this->buffersize = buffersize;
    
    filter = new HighPassFilter();
    filter->coefficients(sampleRate, frequency, resonance);

    
    setSize(120,140);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(18,2);
    
    setName("LP Filter");

    editable = false;
    prefab = true;
}

HPFilterModule::~HPFilterModule()
{

}

void HPFilterModule::configurePins() {
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
    
    Pin* p5 = new Pin(Pin::Type::VALUE);
    p5->direction = Pin::Direction::IN;
    p5->listeners.push_back(this);
    p5->setName("Mod");
    
    addPin(Pin::Direction::IN,p1);
    addPin(Pin::Direction::IN,p2);
    addPin(Pin::Direction::IN,p3);
    addPin(Pin::Direction::OUT,p4);
    addPin(Pin::Direction::IN,p5);
}

void HPFilterModule::paint(juce::Graphics &g) {
    Module::paint(g);
    g.drawImageAt(ImageCache::getFromMemory(BinaryData::hp_png, BinaryData::hp_pngSize),25,40);
}


void HPFilterModule::process() {
    
    if (pins.at(2)->getConnections().size() == 1 && pins.at(2)->getConnections().at(0)->getAudioBuffer() != nullptr && pins.at(3)->getAudioBuffer() != nullptr) {
        const float* in = pins.at(2)->getConnections().at(0)->getAudioBuffer()->getReadPointer(0);
        float* out = pins.at(3)->getAudioBuffer()->getWritePointer(0);
        filter->process(const_cast<float*>(in),out,buffersize);
        for (int i = 0; i < buffersize;i++) {
            pins.at(3)->getAudioBuffer()->getWritePointer(0)[i] = in[i];
        }
    }
    else {
        for (int i = 0; i < buffersize;i++) {
            pins.at(3)->getAudioBuffer()->getWritePointer(0)[i] = 0;
        }
    }
    
    if (pins.at(0)->getConnections().size() ==  1) {
        
        if (this->frequency != pins.at(0)->getConnections().at(0)->getValue()) {
            this->frequency =  pins.at(0)->getConnections().at(0)->getValue();
            filter->coefficients(sampleRate, frequency, resonance);
        }
        
    }
    if (pins.at(1)->getConnections().size() ==  1) {
        
        if (this->resonance != pins.at(1)->getConnections().at(0)->getValue()) {
            this->resonance =  pins.at(1)->getConnections().at(0)->getValue();
            filter->coefficients(sampleRate, frequency, resonance);
        }
    }
    if (pins.at(4)->getConnections().size() ==  1) {
        
        if (this->mod != pins.at(4)->getConnections().at(0)->getValue()) {
            this->mod =  pins.at(4)->getConnections().at(0)->getValue();
            if (this->mod < 0.1)
                this->mod = 0.1;
            filter->coefficients(sampleRate, frequency * mod, resonance);
        }
    }
    
}
