/*
 ==============================================================================
 
 SineModule
 Created: 4 Apr 2018 3:48:23pm
 Author:  Matthias Pueski
 
 ==============================================================================
 */

#include "../JuceLibraryCode/JuceHeader.h"
#include "SineModule.h"
#include "Connection.h"
#include "AudioEngine/Sine.h"

//==============================================================================
SineModule::SineModule(double sampleRate, int buffersize)
{
    this->sampleRate = sampleRate;
    this->buffersize = buffersize;
    
    for (int i = 0; i < 128;i++){
        oscillator[i] = nullptr;
    }
    
    setSize(120,140);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(18,2);
    
    setName("Sine");
    
    editable = false;
    prefab = true;
}

SineModule::~SineModule()
{
    for (int i = 0; i < 128;i++){
        if (oscillator[i] != nullptr)
            delete oscillator[i];
    }
}

void SineModule::configurePins() {
    Pin* p1 = new Pin(Pin::Type::VALUE);
    p1->direction = Pin::Direction::IN;
    p1->listeners.push_back(this);
    p1->setName("P");
    
    Pin* p2 = new Pin(Pin::Type::VALUE);
    p2->direction = Pin::Direction::IN;
    p2->listeners.push_back(this);
    p2->setName("F");
    
    Pin* p3 = new Pin(Pin::Type::VALUE);
    p3->direction = Pin::Direction::IN;
    p3->listeners.push_back(this);
    p3->setName("A");
    
    Pin* p4 = new Pin(Pin::Type::AUDIO);
    p4->direction = Pin::Direction::OUT;
    p4->listeners.push_back(this);
    p4->setName("Out");
    
    Pin* p5 = new Pin(Pin::Type::VALUE);
    p5->direction = Pin::Direction::OUT;
    p5->listeners.push_back(this);
    p5->setName("V");
    
    addPin(Pin::Direction::IN,p1);
    addPin(Pin::Direction::IN,p2);
    addPin(Pin::Direction::IN,p3);
    addPin(Pin::Direction::OUT,p4);
    addPin(Pin::Direction::OUT,p5);
}

void SineModule::paint(juce::Graphics &g) {
    Module::paint(g);
    g.drawImageAt(ImageCache::getFromMemory(BinaryData::sine_png, BinaryData::sine_pngSize),25,40);
}


void SineModule::setPitch(int pitch) {
    this->pitch = pitch;
}

void SineModule::setFine(float fine) {
    if (this->fine != fine) {
        this->fine = fine;
        for (int i = 0; i < 128;i++){
            if (oscillator[i] != nullptr)
                this->oscillator[i]->setFine(fine);
        }
    }
}

void SineModule::setAmplitude(float amplitude) {
    this->amplitude = amplitude;
}

void SineModule::process() {
    bool volumegate = false;
    
    
    if (pins.at(0)->connections.size() ==  1) {
        this->setPitch(pins.at(0)->connections.at(0)->getValue());
    }
    if (pins.at(1)->connections.size() ==  1) {
        this->setFine(pins.at(1)->connections.at(0)->getValue());
    }
    if (pins.at(2)->connections.size() ==  1) {
        volumegate = true;
    }
    if (pins.at(2)->connections.size() == 1) {
        for (int i = 0; i < buffersize; i++) {
            float value = 0;
            for (int j = 0; j < 128;j++){
                
                if(pins.at(2)->connections.at(0)->dataEnabled[j]) {
                    if (oscillator[j] ==  nullptr) {
                        oscillator[j] = new Sine(sampleRate, buffersize);
                        
                        oscillator[j]->setFrequency((440 * pow(2.0,((j+1)-69.0)/12.0)) + pitch);
                    }
                    float volume = pins.at(2)->connections.at(0)->data[j];
                    
                    this->oscillator[j]->setVolume(volume);
                }
                else {
                    if (oscillator[j] != nullptr) {
                        delete this->oscillator[j];
                        this->oscillator[j] = nullptr;
                    }
                }
                if (oscillator[j] != nullptr)
                    value += oscillator[j]->process();
            }
            if (pins.at(3)->getAudioBuffer() != nullptr && pins.at(3)->getAudioBuffer()->getNumChannels() > 0)
                pins.at(3)->getAudioBuffer()->setSample(0,i ,value);
            
            
            //pins.at(4)->setValue(abs(value + 1));
        }
    }
    
}

