/*
  ==============================================================================

    RegionConfig.cpp
    Created: 31 Dec 2016 11:55:34am
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"

#include "RegionConfig.h"

RegionConfig::RegionConfig() {
    
}

RegionConfig::~RegionConfig() {
    delete audioClip;
    delete midiSequence;
}

AudioClip* RegionConfig::getAudioClip() {
    return audioClip;
}

void RegionConfig::setAudioClip(AudioClip* clip) {
    this->audioClip = clip;
}

void RegionConfig::addMessage(MidiMessage* message) {
    midiSequence->addEvent(*message);
}

ValueTree RegionConfig::getConfig() {
    
    ValueTree config = ValueTree("Region");
    
    config.addChild(audioClip->getConfig(), -1, nullptr);
    
    return config;
    
}