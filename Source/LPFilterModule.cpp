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

using juce::Justification;
using juce::ImageCache;
//==============================================================================
LPFilterModule::LPFilterModule(double sampleRate, int buffersize)
{
    this->sampleRate = sampleRate;
    this->buffersize = buffersize;
    
    filter = new LowPassFilter();
    filter->coefficients(sampleRate, frequency, resonance);

    
    setSize(120,140);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(18,2);
    
    setName("LP Filter");

    editable = false;
    prefab = true;
}

LPFilterModule::~LPFilterModule()
{
    delete filter;
}

void LPFilterModule::configurePins() {
    Pin* p1 = new Pin(Pin::Type::VALUE);
    p1->direction = Pin::Direction::IN;
    p1->listeners.push_back(this);
    p1->setName("F");
    
    p1->setDescription("Input for the cutoff frequency of the filter. Typical range [0..15000]");
    
    Pin* p2 = new Pin(Pin::Type::VALUE);
    p2->direction = Pin::Direction::IN;
    p2->listeners.push_back(this);
    p2->setName("R");
    
    p2->setDescription("Resonance or Q-Factor of the filter. Typical range [0..5]");
    
    Pin* p3 = new Pin(Pin::Type::AUDIO);
    p3->direction = Pin::Direction::IN;
    p3->listeners.push_back(this);
    p3->setName("In");
    
    p3->setDescription("Audio in of the filter module.");
    
    Pin* p4 = new Pin(Pin::Type::AUDIO);
    p4->direction = Pin::Direction::OUT;
    p4->listeners.push_back(this);
    p4->setName("Out");
    
    p4->setDescription("Audio out of the filter module.");
    
    Pin* p5 = new Pin(Pin::Type::VALUE);
    p5->direction = Pin::Direction::IN;
    p5->listeners.push_back(this);
    p5->setName("Mod");
    
    p5->setDescription("Modulation in of the filter module. Modulates the cutoff frequency. Connect knobs, oscillators or ADSR modules here. Typical range  [-1..1]");
    
    addPin(Pin::Direction::IN,p1);
    addPin(Pin::Direction::IN,p2);
    addPin(Pin::Direction::IN,p3);
    addPin(Pin::Direction::OUT,p4);
    addPin(Pin::Direction::IN,p5);
}

void LPFilterModule::paint(juce::Graphics &g) {
    Module::paint(g);
    g.drawImageAt(ImageCache::getFromMemory(BinaryData::lp_png, BinaryData::lp_pngSize),25,40);
}


void LPFilterModule::process() {
    
    if (pins.at(2)->getConnections().size() >= 1 && pins.at(2)->getConnections().at(0)->getAudioBuffer() != nullptr && pins.at(3)->getAudioBuffer() != nullptr) {
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
            frequency = std::max(0.0f, std::min(frequency, 20000.0f));
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
            filter->coefficients(sampleRate, 20000.0f * mod - (20000.0f - frequency), resonance);
        }
    }
    
}
