/*
  ==============================================================================

    Pin.cpp
    Created: 1 Apr 2018 6:49:53pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "Pin.h"

Pin::Pin() {
}

void Pin::sendEvent(Event *e) {
    Logger::writeToLog("received event : "+e->getName());
    
    for (int i = 0; i < connections.size();i++) {
        connections.at(i)->sendEvent(new Event(e));
    }
    
    delete e;
}
