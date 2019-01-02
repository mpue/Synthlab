/*
  ==============================================================================

    TrackConfig.cpp
    Created: 31 Dec 2016 11:55:18am
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "TrackConfig.h"

TrackConfig::TrackConfig() {
    
}

TrackConfig::~TrackConfig() {
    for(std::vector<RegionConfig*>::iterator it = regions.begin(); it != regions.end();it++) {
        delete *it;
    }
    for(std::vector<EffectConfig*>::iterator it = effects.begin(); it != effects.end();it++) {
        delete *it;
    }
    for(std::vector<AuxSendConfig*>::iterator it = auxSendConfigs.begin(); it != auxSendConfigs.end();it++) {
        delete *it;
    }
    if (instrument != NULL) {
        delete instrument;
    }
    
}

void TrackConfig::setName(juce::String name) {
    this->name = name;
}

String TrackConfig::getName()
{
    return name;
}

void TrackConfig::setGain(float gain)
{
    this->gain = gain;
}

void TrackConfig::setVolume(float volume)
{
    this->volume = volume;
}

float TrackConfig::getVolume()
{
    return volume;
}

void TrackConfig::setPan(float pan)
{
    this->pan = pan;
}

float TrackConfig::getPan()
{
    return pan;
}

void TrackConfig::setSolo(bool solo) {
    this->solo = solo;
}

bool TrackConfig::isSolo() {
    return solo;
}

void TrackConfig::setMute(bool mute) {
    this->mute = mute;
}

bool TrackConfig::isMute() {
    return mute;
}

void TrackConfig::setMono(bool mono) {
    this->mono = mono;
}

bool TrackConfig::isMono() {
    return mono;
}

void TrackConfig::addRegion(RegionConfig* region) {
    regions.push_back(region);
}

void TrackConfig::addEffect(EffectConfig* effect) {
    effects.push_back(effect);
}

void TrackConfig::addAuxSend(AuxSendConfig* aux) {
    auxSendConfigs.push_back(aux);
}

void TrackConfig::setInstrument(InstrumentConfig* instrument) {
    this->instrument = instrument;
}

void TrackConfig::setMidiChannel(int channel) {
    this->midiChannel = channel;
};

float TrackConfig::getMidiChannel() {
    return this->midiChannel;
}

void TrackConfig::setInputChannels(int* channels) {
    this->inputChannels[0] = channels[0];
    this->inputChannels[1] = channels[1];
}

int* TrackConfig::getInputChannels() {
    return &this->inputChannels[0];
}

void TrackConfig::setOutputChannels(int* channels) {
    this->outputChannels[0] = channels[0];
    this->outputChannels[1] = channels[1];
}

int* TrackConfig::getOutputChannels() {
    return &this->outputChannels[0];
}

String TrackConfig::getType() {
    return this->type;
}

void TrackConfig::setType(String type) {
    this->type = type;
}

void TrackConfig::setMidiInputDevice(String name) {
    this->midiInputDevice = name;
}

void TrackConfig::setMidiOutputDevice(String name) {
    this->midiOutputDevice = name;
}

String TrackConfig::getMidiInputDevice() {
    return midiInputDevice;
}

String TrackConfig::getMidiOutputDevice() {
    return midiOutputDevice;
}

int TrackConfig::getHeight() {
    return height;
}

void TrackConfig::setHeight(int height) {
    this->height = height;
}

vector<RegionConfig*> TrackConfig::getRegions() {
    return regions;
}

ValueTree TrackConfig::getConfig() {
    
    ValueTree config = ValueTree("Track");
    
    config.setProperty("volume", volume, nullptr);
    config.setProperty("gain", gain, nullptr);
    config.setProperty("pan", pan, nullptr);
    config.setProperty("name", name, nullptr);
    config.setProperty("height", height, nullptr);
    config.setProperty("midiChannel", midiChannel, nullptr);
    config.setProperty("solo", solo, nullptr);
    config.setProperty("mute", mute, nullptr);
    config.setProperty("mono", mono, nullptr);
    config.setProperty("inputChannels", String(inputChannels[0])+","+String(inputChannels[1]), nullptr);
    config.setProperty("outputChannels", String(outputChannels[0])+","+String(outputChannels[1]), nullptr);
    config.setProperty("midiInputDevice", midiInputDevice, nullptr);
    config.setProperty("midiOutputDevice", midiOutputDevice, nullptr);
    config.setProperty("type", type, nullptr);
    config.setProperty("midiOutputDevice", midiOutputDevice, nullptr);
    
    for (int i = 0; i < regions.size();i++) {
        config.addChild(regions.at(i)->getConfig(), -1, nullptr);
    }
    
    return config;
}
