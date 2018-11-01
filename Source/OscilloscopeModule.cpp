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

using juce::Justification;
using juce::Line;

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
    
    resizer = new ResizableCornerComponent(this,nullptr);
    resizer->setSize(getWidth(), getHeight());
    addAndMakeVisible(resizer);
    resizer->toFront(true);
    resizer->setTopLeftPosition(getWidth()-10, getHeight()-10);
    resizer->setSize(10,10);
    setInterceptsMouseClicks(false, true);
    resized();
}

OscilloscopeModule::~OscilloscopeModule()
{
    delete resizer;
    stopTimer();
}

void OscilloscopeModule::configurePins() {
    Pin* p1 = new Pin(Pin::Type::AUDIO);
    p1->direction = Pin::Direction::IN;
    p1->listeners.push_back(this);
    p1->setName("In");
    
    Pin* p2 = new Pin(Pin::Type::VALUE);
    p2->direction = Pin::Direction::IN;
    p2->listeners.push_back(this);
    p2->setName("P");
    
    Pin* p3 = new Pin(Pin::Type::VALUE);
    p3->direction = Pin::Direction::IN;
    p3->listeners.push_back(this);
    p3->setName("Xs");
    
    Pin* p4 = new Pin(Pin::Type::VALUE);
    p4->direction = Pin::Direction::IN;
    p4->listeners.push_back(this);
    p4->setName("Ys");
    
    addPin(Pin::Direction::IN,p1);
    addPin(Pin::Direction::IN,p2);
    addPin(Pin::Direction::IN,p3);
    addPin(Pin::Direction::IN,p4);

}

void OscilloscopeModule::paint(juce::Graphics &g) {
    Module::paint(g);
    g.setColour(juce::Colours::black);
    g.fillRect(10,25,getWidth() - 20 ,getHeight() - 40);
    g.setColour(juce::Colours::green);
    
    int sample = 0;
    
    for (int i = 0; i < (buffersize*periods) - 1;i++) {
        sample = i % buffersize;
        float x = 10 + (i * xscale);
        
        g.drawLine(Line<float>(x,buffer[sample] * yscale + getHeight() / 2,10 + (i * xscale),buffer[sample+1]*yscale + getHeight() / 2),2);
    }
}

void OscilloscopeModule::setCurrentLayer(Module::Layer layer) {
    Module::setCurrentLayer(layer);
    
    if (currentLayer == Module::Layer::ALL) {
        setInterceptsMouseClicks(false,false);
    }
    else {
        setInterceptsMouseClicks(false,true);
    }
}

void OscilloscopeModule::resized() {
    if (resizer != nullptr) {
        resizer->setTopLeftPosition(getWidth() - 10, getHeight() - 10);
    }
}

void OscilloscopeModule::process() {
    
    if (pins.at(0)->getConnections().size() == 1 ) {

        const float* in = pins.at(0)->getConnections().at(0)->getAudioBuffer()->getReadPointer(0);
        
        for (int i = 0; i < buffersize;i++) {
            
            if (buffer[currentSample] >= triggerThreshold) {
                trigger = true;
            }
            
            if (trigger || currentSample >=  buffersize) {
                currentSample = 0;
                trigger = false;
                return;
            }
            
            buffer[currentSample] = in[i];
            currentSample = (currentSample + 1) % 1024;
        }
        
    }
    else {
        for (int i = 0; i < buffersize;i++) {
            buffer[currentSample] = 0;
            currentSample = (currentSample + 1) % 1024;
        }
    }
    
    
    if (pins.at(1)->getConnections().size() == 1 ) {
        periods = pins.at(1)->getConnections().at(0)->getValue();
    }
    if (pins.at(2)->getConnections().size() == 1 ) {
        float _xscale = pins.at(2)->getConnections().at(0)->getValue();
        
        if (xscale != _xscale) {
            xscale = _xscale;
        }
        
    }
    if (pins.at(3)->getConnections().size() == 1 ) {
        float _yscale = pins.at(3)->getConnections().at(0)->getValue();
        
        if (yscale != _yscale) {
            yscale = _yscale;
        }
        
    }
}
