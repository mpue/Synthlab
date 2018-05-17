/*
 ==============================================================================
 
 LedModule.cpp
 Created: 16 Apr 2018 12:51:10pm
 Author:  Matthias Pueski
 
 ==============================================================================
 */

#include "LedModule.h"
#include "../JuceLibraryCode/JuceHeader.h"

using juce::Justification;
using juce::String;

//==============================================================================
LedModule::LedModule()
{
    setSize(48,48);
    nameLabel->setVisible(false);
    
    setName("LED");
    editable = false;
    prefab = true;
}

LedModule::~LedModule()
{
}

void LedModule::configurePins() {
    Pin* p1 = new Pin(Pin::Type::VALUE);
    p1->direction = Pin::Direction::IN;
    p1->setName("V");
    addPin(Pin::Direction::IN,p1);
}

void LedModule::paint(juce::Graphics &g) {
    Module::paint(g);
    if (value > 0) {
        g.drawImageAt(ImageCache::getFromMemory(BinaryData::led_on_png, BinaryData::led_on_pngSize).rescaled(32,32),8,8);
    }
    else {
        g.drawImageAt(ImageCache::getFromMemory(BinaryData::led_off_png, BinaryData::led_on_pngSize).rescaled(32, 32),8,8);
    }
    
}

void LedModule::process() {
    
    if (getPins().at(0)->getConnections().size() > 0) {
        if (value != getPins().at(0)->getConnections().at(0)->getValue()) {
           value = getPins().at(0)->getConnections().at(0)->getValue();
            std::function<void(void)> changeLambda =
            [=]() {  repaint(); };
            juce::MessageManager::callAsync(changeLambda);
        }
    }
    
}

