/*
  ==============================================================================

    LabelModule.cpp
    Created: 16 Apr 2018 7:38:38pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "LabelModule.h"
#include "../JuceLibraryCode/JuceHeader.h"

using juce::String;
using juce::Justification;
using juce::Label;

LabelModule::LabelModule() {
    
}

LabelModule::LabelModule(String name) : Module(name)
{
    setSize(120,30);
    nameLabel->setJustificationType (Justification::centred);
    nameLabel->setTopLeftPosition(2,2);
    
    editable = true;
    prefab = true;
}

LabelModule::~LabelModule()
{
}

void LabelModule::configurePins(){
    
}

void LabelModule::setSelected(bool selected) {
    if (selected) {
        nameLabel->setColour(Label::textColourId, juce::Colours::orange);
    }
    else {
        nameLabel->setColour(Label::textColourId, juce::Colours::white);
    }
    Module::setSelected(selected);
}

void LabelModule::paint(juce::Graphics &g) {


}


