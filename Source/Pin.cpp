/*
  ==============================================================================

    Pin.cpp
    Created: 1 Apr 2018 6:49:53pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "Pin.h"
#include <stdio.h>
#include <string.h>

Pin::Pin(Pin::Type type) {
    name = "";
    this->type = type;
    if (type == Pin::Type::AUDIO) {
        audioBuffer = new AudioSampleBuffer(1,512);
    }
}

Pin::~Pin() {
    removeAllChangeListeners();
    if (audioBuffer != nullptr) {
        delete audioBuffer;
    }
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

float Pin::getValue() {
    return value;
}


void Pin::setValue(float value) {
    this->value = value;
}


bool Pin::isSelected() {
    return this->selected;
}

AudioSampleBuffer* Pin::getAudioBuffer() {
    return audioBuffer;
}

void Pin::process(const float *in, float *out, int numSamples) {
    
    if (type == Pin::Type::AUDIO) {
        
        if (direction == Pin::Direction::IN ) {
            for (int j = 0; j < connections.size();j++) {
               // audioBuffer->copyFrom(0, 0, *connections.at(j)->getAudioBuffer(), 0, 0, numSamples);
            }
        }
        /*
        if (direction == Pin::Direction::OUT ) {
            memcpy(out, audioBuffer->getReadPointer(0), numSamples);
        }
         */
        

    }
}
