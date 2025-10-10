/*
  ==============================================================================
    FeedbackModule.cpp
    Created: 10 Oct 2025
    Author:  mpue
  ==============================================================================
*/

#include "FeedbackModule.h"

FeedbackModule::FeedbackModule()
{
    setSize(120, 140);
    setName("Feedback");
    editable = false;
    prefab = true;
    fbValue = initValue;
    setInterceptsMouseClicks(false, true);
    createProperties();
}

FeedbackModule::~FeedbackModule()
{
    delete initialValueListener;
    delete dampingListener;
    delete initialValue;
    delete dampingValue;
}

void FeedbackModule::paint(juce::Graphics& g)
{
    Module::paint(g);
    g.drawImageAt(ImageCache::getFromMemory(BinaryData::infinity_png, BinaryData::infinity_pngSize), 25, 40);
}

void FeedbackModule::process()
{
    float inputValue = fbValue;

    // Check if there's an input connection
    if (pins.at(0)->getConnections().size() == 1) {
        inputValue = pins.at(0)->getConnections().at(0)->getValue();
    }
    else {
        // No input, use initial value
        inputValue = initValue;
    }

    // Apply damping (smoothing)
    fbValue = fbValue * (1.0f - damping) + inputValue * damping;

    // Set the feedback output
    pins.at(1)->setValue(fbValue);

    // Also set the direct output (current value without feedback delay)
    if (pins.at(0)->getConnections().size() == 1) {
        pins.at(2)->setValue(inputValue);
    }
    else {
        pins.at(2)->setValue(initValue);
    }
}

void FeedbackModule::configurePins()
{
    Pin* p1 = new Pin(Pin::Type::VALUE);
    p1->direction = Pin::Direction::IN;
    p1->setName("In");

    Pin* p2 = new Pin(Pin::Type::VALUE);
    p2->direction = Pin::Direction::OUT;
    p2->setName("FB");

    Pin* p3 = new Pin(Pin::Type::VALUE);
    p3->direction = Pin::Direction::OUT;
    p3->setName("Out");

    addPin(p1->direction, p1);
    addPin(p2->direction, p2);
    addPin(p3->direction, p3);
}


juce::Array<juce::PropertyComponent*>& FeedbackModule::getProperties()
{
	properties.clear();

    if (initialProp != nullptr) {
        initialProp = new juce::TextPropertyComponent(*initialValue, "Initial Value", 16, false, true);
        properties.add(initialProp);
    }
    if (dampProp != nullptr) {
        dampProp = new juce::TextPropertyComponent(*dampingValue, "Damping (0-1)", 16, false, true);
        properties.add(dampProp);
    }

    return properties;
}

void FeedbackModule::createProperties()
{
    initialValue = new juce::Value(initValue);
    initialValueListener = new InitialValueListener(*initialValue, this);

    dampingValue = new juce::Value(damping);
    dampingListener = new DampingListener(*dampingValue, this);

    dampProp = new juce::TextPropertyComponent(*dampingValue, "Damping (0-1)", 16, false, true);
    initialProp = new juce::TextPropertyComponent(*initialValue, "Initial Value", 16, false, true);    
}