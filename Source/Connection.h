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
class Module;
class Connection
{
public:
	Connection();
	Connection(Module* source, Pin* output,	Module* target, Pin* input);
	~Connection();
	Module* source = nullptr;
	Pin* a;
	Module* target = nullptr;
	Pin* b;
	bool selected = false;
};

