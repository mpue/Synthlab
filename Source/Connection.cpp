/*
  ==============================================================================

    Connection.cpp
    Created: 2 Apr 2018 5:18:02pm
    Author:  mpue

  ==============================================================================
*/

#include "Connection.h"

Connection::Connection()
{
}

Connection::Connection(Module* source, Pin* output, Module* target, Pin* input) {
	this->source = source;
	this->target = target;
	this->input = input;
	this->output = output;
}

Connection::~Connection()
{
}

