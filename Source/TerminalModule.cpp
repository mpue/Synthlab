/*
 ==============================================================================
 
 TerminalModule
 Created: 4 Apr 2018 3:48:23pm
 Author:  Matthias Pueski
 
 ==============================================================================
 */

#include "../JuceLibraryCode/JuceHeader.h"
#include "TerminalModule.h"


//==============================================================================
TerminalModule::TerminalModule(Direction dir) : Module("T"), direction(dir)
{
    setName("T");
    setSize(120,30);
    nameLabel->setJustificationType (Justification::left);
    
    if (direction == TerminalModule::Direction::IN) {
        nameLabel->setTopLeftPosition(2,2);
    }
    else {
        nameLabel->setTopRightPosition(20, 2);
    }
    
    editable = false;
    prefab = true;
    
    
}


void TerminalModule::configurePins(){
    
    Pin* p1 = new Pin(Pin::Type::VALUE);
    
    if (direction == TerminalModule::Direction::IN) {
        p1->direction = Pin::Direction::OUT;
        p1->setName("Out");
        addPin(Pin::Direction::OUT,p1);
    }
    else {
        p1->direction = Pin::Direction::IN;
        p1->setName("In");
        addPin(Pin::Direction::IN,p1);
    }
    
    
    
}

void TerminalModule::paint(juce::Graphics &g) {
    Module::paint(g);
}

TerminalModule::Direction TerminalModule::getDirection() {
    return direction;
}

void TerminalModule::setDirection(TerminalModule::Direction dir) {
    this->direction = dir;
}

