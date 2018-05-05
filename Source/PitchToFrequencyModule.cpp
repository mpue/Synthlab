/*
  ==============================================================================

    PitchToFrequencyModule.cpp
    Created: 16 Apr 2018 5:55:24pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "PitchToFrequencyModule.h"

using juce::Justification;
using juce::ImageCache;

PitchToFrequencyModule::PitchToFrequencyModule()
{
    setSize(120,80);
    nameLabel->setJustificationType (Justification::centred);
    nameLabel->setTopLeftPosition(2,2);
    nameLabel->setText("P2F", juce::NotificationType::dontSendNotification);
    editable = false;
    prefab = true;
}

PitchToFrequencyModule::~PitchToFrequencyModule()
{
}

void PitchToFrequencyModule::configurePins(){
    Pin* p1 = new Pin(Pin::Type::VALUE);
    p1->direction = Pin::Direction::IN;
    p1->setName("In");
    addPin(Pin::Direction::IN,p1);
    
    Pin* p2 = new Pin(Pin::Type::VALUE);
    p2->direction = Pin::Direction::OUT;
    p2->setName("Out");
    addPin(Pin::Direction::OUT,p2);
}

void PitchToFrequencyModule::paint(juce::Graphics &g) {
    Module::paint(g);
    g.drawImageAt(ImageCache::getFromMemory(BinaryData::p2f_png, BinaryData::p2f_pngSize),25,40);
}

void PitchToFrequencyModule::process() {
    
    if (pins.at(0)->getConnections().size() == 1) {
        int value = pins.at(0)->getConnections().at(0)->getValue();
        pins.at(1)->setValue((440 * pow(2.0,(value-69.0)/12.0)));
    }
    
}
