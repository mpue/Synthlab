/*
  ==============================================================================

    Sample.cpp
    Created: 31 Dec 2016 10:00:19am
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "AudioClip.h"

AudioClip::AudioClip(String refId, String name, long length, long offset) {
    this->refId = refId;
    this->length = length;
    this->offset = offset;
    this->name = name;
}

AudioClip::AudioClip() {

}

AudioClip::~AudioClip() {
    
}

void AudioClip::setLength(long length) {
    this->length = length;
}

long AudioClip::getLength() {
    return length;
}

void AudioClip::setOffset(long offset) {
    this->offset = offset;
}

long AudioClip::getOffset() {
    return offset;
}

void AudioClip::setName(juce::String name) {
    this->name = name;
}

String AudioClip::getName() {
    return name;
}

void AudioClip::setRefId(juce::String id) {
    this->refId = id;
}

String AudioClip::getRefId() {
    return refId;
}

ValueTree AudioClip::getConfig() {
    
    ValueTree config = ValueTree("AudioClip");
    
    config.setProperty("name", name, nullptr);
    config.setProperty("refId", refId, nullptr);
    config.setProperty("length", (int)length, nullptr);
    config.setProperty("offset", (int)offset, nullptr);

    return config;
}
