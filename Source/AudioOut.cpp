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


//==============================================================================
AudioOut::AudioOut()
{
    setSize(120,70);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(18,2);
    setName("Audio out");
    Pin* p1 = new Pin(Pin::Type::AUDIO);
    p1->direction = Pin::Direction::IN;
    p1->setName("L");
    
    Pin* p2 = new Pin(Pin::Type::AUDIO);
    p2->direction = Pin::Direction::IN;
    p2->setName("R");
    
    addPin(Pin::Direction::IN,p1);
    addPin(Pin::Direction::IN,p2);
    
    editable = false;
}

AudioOut::~AudioOut()
{
    
 
}

void AudioOut::paint(juce::Graphics &g) {
    Module::paint(g);
}

void AudioOut::process() {
    
}

