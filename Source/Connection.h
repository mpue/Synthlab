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
class Connection : public Component
{
public:
    void paint (Graphics& g) override;
    void resized() override;
	Connection();
	Connection(Module* source, Pin* output,	Module* target, Pin* input);
	~Connection();
	Module* source = nullptr;
	Pin* a;
	Module* target = nullptr;
	Pin* b;
	bool selected = false;
    void setPoints(Point<int>& p1, Point<int>& p2);
    Path* getPath();
private:
    ScopedPointer<Path> linePath = nullptr;
    Point<int> p1;
    Point<int> p2;
};

