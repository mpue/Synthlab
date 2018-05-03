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
        audioBuffer = new AudioSampleBuffer(2,1024);
    }
}

Pin::~Pin() {
    removeAllChangeListeners();
    
    if (audioBuffer != nullptr) {
        audioBuffer->clear();
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
    if (terminal != nullptr) {
        if (terminal->connections.size() == 1) {
            return terminal->connections.at(0)->getValue();
        }
        
    }
    return value;
}


void Pin::setValue(float value) {
    
    if (terminal != nullptr) {
        terminal->setValue(value);
    }
    else {
        this->value = value;
    }
}


bool Pin::isSelected() {
    return this->selected;
}

AudioSampleBuffer* Pin::getAudioBuffer() {
    return audioBuffer;
}

void Pin::process(const float *in, float *out, int numSamples) {
    

}

bool Pin::hasConnection(Pin *target) {
    for (int i = 0; i < connections.size();i++) {
        if (connections.at(i)->index == target->index) {
            return true;
        }
    }
    return false;
}

void Pin::setNote(int note) {
    this->note = note;
}

int Pin::getNote() {
    return note;
}
