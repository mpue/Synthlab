/*
  ==============================================================================

    Pin.cpp
    Created: 1 Apr 2018 6:49:53pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "Pin.h"

Pin::Pin(Pin::Type type) {
    name = "";
    this->type = type;
    if (type == Pin::Type::AUDIO) {
        audioBuffer = new AudioSampleBuffer(1,512);
    }
}

Pin::~Pin() {
    removeAllChangeListeners();
}

void Pin::sendEvent(Event *e) {
    
    // Logger::writeToLog("received event : "+e->getName() + " value "+String(e->getValue()));
    
    for (int i = 0; i < connections.size();i++) {
        connections.at(i)->sendEvent(new Event(e));
    }
    
    for (int i = 0; i < listeners.size();i++) {
        listeners.at(i)->eventReceived(new Event(e));
    }
    
    delete e;
}

String Pin::getName() {
    return name;
}

void Pin::setName(String name) {
    this->name = name;
}

void Pin::setSelected(bool selected) {
    this->selected = selected;
    sendChangeMessage();
}

bool Pin::isSelected() {
    return this->selected;
}

AudioSampleBuffer* Pin::getAudioBuffer() {
    return audioBuffer;
}

void Pin::process(float *in, float *out, int numSamples) {
    
    if (type == Pin::Type::AUDIO) {
        
        if (direction == Pin::Direction::IN ) {
            for (int i = 0; i < numSamples;i++){
                audioBuffer->addSample(0, i, in[i]);
            }
        }
        else {
            for (int i = 0; i < numSamples;i++){
                out[i] = audioBuffer->getSample(0, i);
            }
        }
        
        for (int i = 0; i < connections.size();i++) {
            if (connections.at(i)->direction == Pin::Direction::OUT) {
                connections.at(i)->process(in, out, numSamples);
            }
        }
    }
}
