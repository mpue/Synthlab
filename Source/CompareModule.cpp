/*
  ==============================================================================

    CompareModule.cpp
    Created: 24 Aug 2018 12:53:21pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "CompareModule.h"

#include "../JuceLibraryCode/JuceHeader.h"

#include "Connection.h"

using juce::Justification;
using juce::ImageCache;

//==============================================================================
CompareModule::CompareModule()
{
    setSize(120,140);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(18,2);
    setName("Compare");
    editable = false;
    prefab = true;
    compareModeCombo = new ComboBox();

    compareModeCombo->setBounds(20,getHeight() - 40,getWidth() - 40 , 30);
    compareModeCombo->addItem("EQUALS", 1);
    compareModeCombo->addItem("A > B", 2);
    compareModeCombo->addItem("A < B ", 3);
    compareModeCombo->addListener(this);
    addAndMakeVisible(compareModeCombo);
    compareModeCombo->toFront(true);
    setInterceptsMouseClicks(false,true);
}

CompareModule::~CompareModule()
{
    delete compareModeCombo;
}

void CompareModule::configurePins() {
    Pin* p1 = new Pin(Pin::Type::VALUE);
    p1->direction = Pin::Direction::IN;
    p1->setName("A");
    
    Pin* p2 = new Pin(Pin::Type::VALUE);
    p2->direction = Pin::Direction::IN;
    p2->setName("B");
    
    Pin* p3 = new Pin(Pin::Type::VALUE);
    p3->direction = Pin::Direction::OUT;
    p3->setName("C");
    
    addPin(Pin::Direction::IN,p1);
    addPin(Pin::Direction::IN,p2);
    addPin(Pin::Direction::OUT,p3);
}

void CompareModule::paint(juce::Graphics &g) {
    Module::paint(g);
    // g.drawImageAt(ImageCache::getFromMemory(BinaryData::module_png, BinaryData::module_pngSize),25,40);
}

void CompareModule::setMode(int mode) {
    this->mode = mode;
    this->compareModeCombo->setSelectedId(mode);
}

int CompareModule::getMode() {
    return this->mode;
}

void CompareModule::process() {
    
    float a = 0;
    float b = 0;
    float c = 0;
    
    if (pins.at(0)->getConnections().size() > 0) {
        a = pins.at(0)->getConnections().at(0)->getValue();
    }
    if (pins.at(1)->getConnections().size() > 0) {
        b = pins.at(1)->getConnections().at(0)->getValue();
    }
    
    if (compareModeCombo->getSelectedId() == 1) {
        if (a == b) {
            c = 1;
        }
    }
    else if (compareModeCombo->getSelectedId() == 2) {
        
        if (a > b) {
            c = 1;
        }
    }
    else if (compareModeCombo->getSelectedId() == 3) {
    
        if (a < b) {
            c = 1;
        }
    }
    
    pins.at(2)->setValue(c);
    
}

void CompareModule::comboBoxChanged(juce::ComboBox *comboBoxThatHasChanged) {
    mode = comboBoxThatHasChanged->getSelectedId();
}
