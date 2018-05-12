/*
  ==============================================================================

    Connection.h
    Created: 2 Apr 2018 5:18:02pm
    Author:  mpue

  ==============================================================================
*/

#pragma once

// #include "Module.h"
#include "Pin.h"
#include "../JuceLibraryCode/JuceHeader.h"
class Module;
class Connection : public juce::Component
{
public:
    void paint (juce::Graphics& g) override;
    void resized() override;
	Connection();
	Connection(Module* source, Pin* output,	Module* target, Pin* input);
	~Connection();
	Module* source = nullptr;
	Pin* a;
	Module* target = nullptr;
	Pin* b;
	bool selected = false;
    void setPoints(juce::Point<int>& p1, juce::Point<int>& p2);
    juce::Path* getPath();
private:
    juce::Path* linePath;
    juce::Point<int> p1;
    juce::Point<int> p2;
};

