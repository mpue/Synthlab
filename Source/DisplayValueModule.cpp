/*
  ==============================================================================

    DisplayValueModule.cpp
    Created: 16 Apr 2018 12:51:10pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "DisplayValueModule.h"
#include "../JuceLibraryCode/JuceHeader.h"



//==============================================================================
DisplayValueModule::DisplayValueModule()
{
    setSize(120,140);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(18,2);
    setName("Display value");
    editable = false;
    prefab = true;
}

DisplayValueModule::~DisplayValueModule()
{
}

void DisplayValueModule::configurePins() {
    Pin* p1 = new Pin(Pin::Type::VALUE);
    p1->direction = Pin::Direction::IN;
    p1->setName("V");
    addPin(Pin::Direction::IN,p1);
}

void DisplayValueModule::paint(juce::Graphics &g) {
    Module::paint(g);
    g.setFont(16);
    g.setColour(juce::Colours::white);
    g.drawText(String(value), getWidth() / 3  , getHeight() / 3, getWidth() * 0.8, getHeight() / 3, juce::Justification::centred);
}

void DisplayValueModule::process() {
    
    if (pins.at(0)->connections.size() > 0) {
        value = pins.at(0)->connections.at(0)->getValue();
    }
    
}

