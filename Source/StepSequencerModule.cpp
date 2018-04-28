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
    setSize(240,120);
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
    p1->direction = Pin::Direction::IN;
    p1->listeners.push_back(this);
    p1->setName("G");
    
    Pin* p2 = new Pin(Pin::Type::EVENT);
    p2->direction = Pin::Direction::OUT;
    p2->listeners.push_back(this);
    p2->setName("G");
    
    addPin(Pin::Direction::IN,p1);
    addPin(Pin::Direction::OUT,p2);
    
    editor = new SequenceEditor(pins.at(1));
}

SequenceEditor* StepSequencerModule::getEditor() {
    if (editor == nullptr) {
        editor = new SequenceEditor(pins.at(1));
    }
    return editor;
}

void StepSequencerModule::paint(juce::Graphics &g) {
    Module::paint(g);
    g.drawImageAt(ImageCache::getFromMemory(BinaryData::sequence_png, BinaryData::sequence_pngSize),25,40);
}

void StepSequencerModule::process() {

    
}


void StepSequencerModule::eventReceived(Event *e) {
    
    if (e->getType() == Event::Type::GATE) {
        
        // trigger event  changes state of the envelopes
        
        if (e->getValue() > 0) {
            editor->handleIncomingMidiMessage(nullptr,MidiMessage::noteOn(1, e->getNote(), 1.0f)); 
        }
        else {
        }
    }
    
}



