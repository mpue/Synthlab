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
TerminalModule::TerminalModule(Direction dir, Type type) : Module("T"), direction(dir)
{


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
    
    createProperties();
    setType(type);
    setName("T");
}

TerminalModule::~TerminalModule() {
    delete nameListener;
    delete typeListener;
    delete nameValue;
    delete typeValue;
}

void TerminalModule::setName(juce::String name) {
    Module::setName(name);
    nameValue->setValue(name);
    sendChangeMessage();
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

void TerminalModule::setType(TerminalModule::Type type) {
    Terminal::setType(type);
    this->typeValue->setValue(type);
    if (this->pins.size() > 0)
        this->pins.at(0)->setType(type);
    sendChangeMessage();
}


void TerminalModule::createProperties() {
    nameValue = new Value();
    nameListener = new NameListener(*nameValue, this);
    
    typeValue = new Value();
    typeListener = new TypeListener(*typeValue, this);
}

juce::Array<PropertyComponent*>& TerminalModule::getProperties() {
    nameProp = new TextPropertyComponent(*nameValue,"Name",16, false,true);
    
    Array<var> values = Array<var>();
    
    values.add(Type::AUDIO);
    values.add(Type::EVENT);
    values.add(Type::VALUE);

    StringArray types = StringArray();
    types.add("Audio");
    types.add("Event");
    types.add("Value");
    
    
    typeProp = new ChoicePropertyComponent(*typeValue,"Type",types,values);

    properties = juce::Array<PropertyComponent*>();
    properties.add(nameProp);
    properties.add(typeProp);
    return properties;
}
