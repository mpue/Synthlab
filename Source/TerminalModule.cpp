/*
 ==============================================================================
 
 TerminalModule
 Created: 4 Apr 2018 3:48:23pm
 Author:  Matthias Pueski
 
 ==============================================================================
 */

#include "../JuceLibraryCode/JuceHeader.h"
#include "TerminalModule.h"
#include "Pin.h"

using juce::Justification;
using juce::Value;
using juce::PropertyComponent;
using juce::TextPropertyComponent;
using juce::Array;
using juce::var;
using juce::String;
using juce::ChoicePropertyComponent;
using juce::StringArray;
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
    removeAllChangeListeners();
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
        p1->setName("In");
        //p1->listeners.push_back(this);
        addPin(Pin::Direction::OUT,p1);
    }
    else {
        p1->direction = Pin::Direction::IN;
        p1->setName("Out");
        // p1->listeners.push_back(this);
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

void TerminalModule::process() {
    Module::process();
    for (int i = 0; i < buffersize;i++) {
        if (getType() == Terminal::Type::AUDIO){
            if (direction == Terminal::Direction::OUT){
                if (getPins().at(0)->getTerminal() != nullptr) {
                    if (getPins().at(0)->getConnections().size() == 1) {
                        const float* source = getPins().at(0)->getConnections().at(0)->getAudioBuffer()->getReadPointer(0);
                        float* target = getPins().at(0)->getTerminal()->getAudioBuffer()->getWritePointer(0);
                        if (target != NULL) {
                            target[i] = source[i];
                        }
                    }
                }
            }
            
            else {
                if (getPins().at(0)->getTerminal() != nullptr) {
                    
                    float* target = getPins().at(0)->getAudioBuffer()->getWritePointer(0);
                    
                    if (target != NULL) {
                        if (getPins().at(0)->getTerminal()->getConnections().size() == 1) {
                            const float* source = getPins().at(0)->getTerminal()->getConnections().at(0)->getAudioBuffer()->getReadPointer(0);
                            target[i] = source[i];
                        }
                        else {
                            target[i] = 0;
                        }
                    }
                    

                }
            }
            
        }
    }

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

void TerminalModule::eventReceived(Event *e) {
    pins.at(0)->sendEvent(e);
}
