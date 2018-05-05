/*
 ==============================================================================
 
 MidiGate.cpp
 Created: 4 Apr 2018 3:48:23pm
 Author:  Matthias Pueski
 
 ==============================================================================
 */

#include "../JuceLibraryCode/JuceHeader.h"
#include "AudioIn.h"
#include "Connection.h"

using juce::Justification;
using juce::ImageCache;

//==============================================================================
AudioIn::AudioIn()
{
    setSize(120,140);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(18,2);
    setName("Audio in");
    
    editable = false;
    prefab = true;
}

AudioIn::~AudioIn()
{
    
    
}

void AudioIn::configurePins() {
    Pin* p1 = new Pin(Pin::Type::AUDIO);
    p1->direction = Pin::Direction::IN;
    p1->setName("L");
    
    Pin* p2 = new Pin(Pin::Type::AUDIO);
    p2->direction = Pin::Direction::IN;
    p2->setName("R");
    
    addPin(Pin::Direction::OUT,p1);
    addPin(Pin::Direction::OUT,p2);
    
}

void AudioIn::paint(juce::Graphics &g) {
    Module::paint(g);
    g.drawImageAt(ImageCache::getFromMemory(BinaryData::micro_png, BinaryData::micro_pngSize),25,40);
}

void AudioIn::process() {
    
}

void AudioIn::setChannelIndex(int index) {
    this->channelIndex = index;
}

int AudioIn::getChannelIndex() {
    return channelIndex;
}



