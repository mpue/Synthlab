//
//  Event.cpp
//  Synthlab - App
//
//  Created by Matthias Pueski on 05.04.18.
//

#include "Event.h"

using juce::String;

Event::Event() {
    
}

Event::Event(Event* e){
    this->name = e->getName();
    this->type = e->getType();
    this->value = e->getValue();
    this->note = e->getNote();
}

Event::Event(String name, Type type) {
    this->name = name;
    this->type = type;
}

Event::~Event() {
    
}

String Event::getName() {
    return name;
}

Event::Type Event::getType() {
    return type;
}

void Event::setValue(int value) {
    this->value = value;
}

int Event::getValue() {
    return value;
}

void Event::setNote(int note) {
    this->note = note;
}

int Event::getNote(){
    return note;
}

void Event::setNumber(int num) {
    this->number = num;
}

int Event::getNumber() {
    return number;
}
