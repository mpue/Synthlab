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

Connection::Connection(Module* source, Pin* a, Module* target, Pin* b) {
	this->source = source;
	this->target = target;
	this->a = a;
	this->b = b;
}

Connection::~Connection()
{
}

