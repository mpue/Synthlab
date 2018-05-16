/*
  ==============================================================================

    Topic.cpp
    Created: 16 May 2018 8:52:28pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "Topic.h"

Topic::Topic() {
    
}

Topic::~Topic() {
    
}

juce::String Topic::getName() {
    return name;
}

void Topic::setName(juce::String name) {
    this->name = name;
}

float Topic::getValue() {
    return value;
}

void Topic::setValue(float value) {
    this->value = value;
}
