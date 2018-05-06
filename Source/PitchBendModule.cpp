/*
 ==============================================================================
 
 PitchBendModule.cpp
 Created: 4 Apr 2018 3:48:23pm
 Author:  Matthias Pueski
 
 ==============================================================================
 */

#include "../JuceLibraryCode/JuceHeader.h"
#include "PitchBendModule.h"
#include "Connection.h"

using juce::Justification;
using juce::ImageCache;

//==============================================================================
PitchBendModule::PitchBendModule()
{
    // In your constructor, you should add any child components, and
    // initialise any special settings that your component needs.
    
    setName("Pitch bend");
    setSize(120,140);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(2,2);
    editable = false;
    prefab = true;
}

PitchBendModule::~PitchBendModule()
{
}

void PitchBendModule::configurePins() {
    
    Pin* p1 = new Pin(Pin::Type::VALUE);
    p1->direction = Pin::Direction::OUT;
    p1->listeners.push_back(this);
    p1->setName("P");
    p1->setValue(1);
    addPin(Pin::Direction::OUT,p1);
    
    Pin* p2 = new Pin(Pin::Type::EVENT);
    p2->direction = Pin::Direction::OUT;
    p2->listeners.push_back(this);
    p2->setName("E");
    addPin(Pin::Direction::OUT,p2);
}

void PitchBendModule::paint(juce::Graphics &g) {
    Module::paint(g);
    g.drawImageAt(ImageCache::getFromMemory(BinaryData::midiin_png, BinaryData::midiin_pngSize),25,40);
}

void PitchBendModule::setPitch(int value) {
    
    float nPitch = ((float)value - (float)0x3fff/2.0) / 8192.0f;
    float semitones = 4.0f;
    
    nPitch = (nPitch * semitones) / 12.0f;
    nPitch = pow(2, nPitch);
    
    for (int i = 0; i < 128;i++) {
        pins.at(0)->data[i] = nPitch;
    }
    
    pins.at(0)->setValue(nPitch);
    
    Event* e = new Event("pitch",Event::Type::PITCH);
    e->setValue(nPitch);
    pins.at(1)->sendEvent(e);
    
}


