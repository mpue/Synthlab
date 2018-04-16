/*
  ==============================================================================

    OscilloscopeModule.cpp
    Created: 16 Apr 2018 3:14:04pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "OscilloscopeModule.h"

#include "../JuceLibraryCode/JuceHeader.h"
#include "Connection.h"


//==============================================================================
OscilloscopeModule::OscilloscopeModule(double sampleRate, int buffersize)
{
    this->sampleRate = sampleRate;
    this->buffersize = buffersize;
    
    for (int i = 0; i < buffersize;i++) {
        this->buffer[i] = 0;
    }

    
    setSize(140,140);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(18,2);
    setName("Scope");
    
    editable = false;
    prefab = true;
    startTimer(100);
}

OscilloscopeModule::~OscilloscopeModule()
{
    stopTimer();
}

void OscilloscopeModule::configurePins() {
    Pin* p1 = new Pin(Pin::Type::AUDIO);
    p1->direction = Pin::Direction::IN;
    p1->listeners.push_back(this);
    p1->setName("In");
    
    addPin(Pin::Direction::IN,p1);

}

void OscilloscopeModule::paint(juce::Graphics &g) {
    Module::paint(g);
    g.setColour(juce::Colours::black);
    g.fillRect(10,25,120,100);
    g.setColour(juce::Colours::green);
    
    int periods = buffersize / 120;
    
    int sample = 0;
    
    for (int i = 0; i < (buffersize*periods) - 1;i++) {
        sample = i % buffersize;
        g.drawLine(Line<float>(10 + i,buffer[sample] * yscale + 75,10 + (i * xscale),buffer[sample+1]*yscale + 75), 1);
    }
}


void OscilloscopeModule::process() {
    
    if (pins.at(0)->connections.size() == 1 ) {
        const float* in = pins.at(0)->connections.at(0)->getAudioBuffer()->getReadPointer(0);
        for (int i = 0; i < buffersize;i++) {
            buffer[currentSample] = in[i];
            currentSample = (currentSample + 1) % 1024;
        }
        
    }
    
}
