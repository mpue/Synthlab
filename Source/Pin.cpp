/*
  ==============================================================================

    Pin.cpp
    Created: 1 Apr 2018 6:49:53pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "Pin.h"

Pin::Pin() {
    name = "";
}

void Pin::sendEvent(Event *e) {
    
    // Logger::writeToLog("received event : "+e->getName() + " value "+String(e->getValue()));
    
    for (int i = 0; i < connections.size();i++) {
        connections.at(i)->sendEvent(new Event(e));
    }
    
    for (int i = 0; i < listeners.size();i++) {
        listeners.at(i)->eventReceived(new Event(e));
    }
    
    delete e;
}

String Pin::getName() {
    return name;
}

void Pin::setName(String name) {
    this->name = name;
}
