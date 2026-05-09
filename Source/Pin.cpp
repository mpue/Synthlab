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

using juce::AudioSampleBuffer;
using juce::String;
using juce::Component;

Pin::Pin(Terminal::Type type) {
    name = "";
    this->type = type;
    // if (type == Terminal::Type::AUDIO) {
        audioBuffer = new AudioSampleBuffer(2,1024);
   //  }
}

Pin::~Pin() {
    removeAllChangeListeners();
    
    if (audioBuffer != nullptr) {
        audioBuffer->clear();
        delete audioBuffer;
    }
    
}

void Pin::sendEvent(Event *e) {

    noteEvent();

    // Source-Pin ans Event haengen, damit Empfaenger mit mehreren EVENT-IN-
    // Pins unterscheiden koennen, wer gefeuert hat. Jeder Pin in der Kette
    // ueberschreibt das Feld auf sich selbst — der Listener sieht immer den
    // unmittelbar liefernden Pin (= einen seiner eigenen IN-Pins).
    e->setSource(this);

    for (int i = 0; i < connections.size();i++) {
        connections.at(i)->sendEvent(new Event(e));
    }

    for (int i = 0; i < listeners.size();i++) {
        listeners.at(i)->eventReceived(new Event(e));
    }

    delete e;
}

void Pin::noteEvent() {
    lastEventTimeMs = juce::Time::getMillisecondCounter();
}

float Pin::getEventGlow(juce::int64 nowMs, juce::int64 fadeMs) const {
    if (lastEventTimeMs == 0) return 0.0f;
    const juce::int64 delta = nowMs - lastEventTimeMs;
    if (delta < 0 || delta >= fadeMs) return 0.0f;
    return 1.0f - (float)delta / (float)fadeMs;
}

void Pin::updateAudioPeak() {
    if (audioBuffer == nullptr
        || audioBuffer->getNumChannels() == 0
        || audioBuffer->getNumSamples() == 0) {
        audioLevel *= 0.85f;
        if (audioLevel < 1.0e-4f) audioLevel = 0.0f;
        return;
    }
    const float peak = audioBuffer->getMagnitude(0, 0, audioBuffer->getNumSamples());
    if (peak > audioLevel) {
        audioLevel = peak;
    } else {
        audioLevel *= 0.85f;
        if (audioLevel < 1.0e-4f) audioLevel = 0.0f;
    }
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

juce::Vector3D<float> Pin::getVector3D()
{
    return vector3D;
}
void Pin::setVector3D(juce::Vector3D<float> vec) {
    this->vector3D = vec;

}
Geometry* Pin::getGeometry()
{
    return geometry;;
}
std::vector<Pin*>& Pin::getConnections() {
    return connections;
}

bool Pin::isSelected() {
    return this->selected;
}

AudioSampleBuffer* Pin::getAudioBuffer() {
    return audioBuffer;
}


void Pin::setAudioBuffer(AudioSampleBuffer* buffer) {
    this->audioBuffer = buffer;
}

void Pin::process(const float *in, float *out, int numSamples) {
    

}

bool Pin::hasConnection(Pin *target) {
    for (int i = 0; i < getConnections().size();i++) {
        if (getConnections().at(i)->index == target->index) {
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

void Pin::changeListenerCallback(juce::ChangeBroadcaster *source) {
    
    Component* c = nullptr;
    
    if ((c = dynamic_cast<Component*>(source)) != nullptr) {
        setName(c->getName());
    }
    
    Terminal* t = nullptr;
    
    if ((t = dynamic_cast<Terminal*>(source)) != nullptr) {
        setType(t->getType());
    }
    
}

void Pin::addEventListener(EventListener* listener)
{
    listeners.push_back(listener);
}
