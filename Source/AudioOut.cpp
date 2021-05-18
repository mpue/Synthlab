/*
  ==============================================================================

    MidiGate.cpp
    Created: 4 Apr 2018 3:48:23pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "AudioOut.h"
#include "Connection.h"

using juce::Justification;
using juce::ImageCache;

//==============================================================================
AudioOut::AudioOut()
{
    setSize(120,140);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(18,2);
    setName("Audio out");

    editable = false;
    prefab = true;
    
    createProperties();
}

AudioOut::~AudioOut()
{
    delete volumeValue;
    delete volumeListener;
}

void AudioOut::createProperties() {
    volumeValue = new Value();
    volumeListener = new ValueListener(*volumeValue, this);
}

void AudioOut::configurePins() {
    Pin* p1 = new Pin(Pin::Type::AUDIO);
    p1->direction = Pin::Direction::IN;
    p1->setName("L");
    
    Pin* p2 = new Pin(Pin::Type::AUDIO);
    p2->direction = Pin::Direction::IN;
    p2->setName("R");
    
    addPin(Pin::Direction::IN,p1);
    addPin(Pin::Direction::IN,p2);
    
}

void AudioOut::paint(juce::Graphics &g) {
    Module::paint(g);
    g.drawImageAt(ImageCache::getFromMemory(BinaryData::speaker_png, BinaryData::speaker_pngSize),25,40);
}

void AudioOut::process() {
    
}

void AudioOut::setChannelIndex(int index) {
    this->channelIndex = index;
}

int AudioOut::getChannelIndex() {
    return channelIndex;
}

juce::Array<PropertyComponent*>& AudioOut::getProperties() {
    
    properties = juce::Array<PropertyComponent*>();
    volumeProp = new SliderPropertyComponent(*volumeValue,"Volume",0,1,0.01,1.0,true);
    
    properties.add(volumeProp);
    
    return properties;
}

