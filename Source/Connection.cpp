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
    a->connections.clear();
    b->connections.clear();
    
    
    for (std::vector<Pin*>::iterator it = a->connections.begin(); it != a->connections.end();it++) {
        (*it)->invalidate();
    }
    for (std::vector<Pin*>::iterator it = b->connections.begin(); it != b->connections.end();it++) {
        (*it)->invalidate();
    }
     
}

