/*
 ==============================================================================
 
 MidiGate.cpp
 Created: 4 Apr 2018 3:48:23pm
 Author:  Matthias Pueski
 
 ==============================================================================
 */

#include "../JuceLibraryCode/JuceHeader.h"
#include "StepSequencerModule.h"
#include "Connection.h"


//==============================================================================
StepSequencerModule::StepSequencerModule(float sampleRate, int bufferSize)
{
    setSize(120,140);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(18,2);
    setName("Step sequencer");
    editable = false;
    prefab = true;
    this->buffersize = bufferSize;
    this->sampleRate = sampleRate;
}

StepSequencerModule::~StepSequencerModule()
{
}

void StepSequencerModule::configurePins() {


    Pin* p1 = new Pin(Pin::Type::EVENT);
    p1->direction = Pin::Direction::OUT;
    p1->setName("G");
    

    addPin(Pin::Direction::OUT,p1);
}

void StepSequencerModule::paint(juce::Graphics &g) {
    Module::paint(g);
    // g.drawImageAt(ImageCache::getFromMemory(BinaryData::StepSequencer_png, BinaryData::StepSequencer_pngSize),25,40);
}

void StepSequencerModule::process() {

    
    
    
}


