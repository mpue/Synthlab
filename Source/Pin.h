/*
  ==============================================================================

    Pin.h
    Created: 1 Apr 2018 6:49:53pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once

#include "AudioEngine/Event.h"
#include "AudioEngine/EventListener.h"
#include "../JuceLibraryCode/JuceHeader.h"

class Pin : public ChangeBroadcaster {


public:

    Pin();
    ~Pin();
    
	static enum Direction{
		IN,
		OUT
	};

    static enum Type {
        AUDIO,
        EVENT
    };
    
    int x;
    long index;
    int y;

	Direction direction;
    Type type;
    
    void setSelected(bool selected);
    bool isSelected();
    
    std::vector<Pin*> connections;
    std::vector<EventListener*> listeners;
    
    void sendEvent(Event* e);
    
    String getName();
    void setName(String name);
    
private:
    bool selected = false;
    String name;
    
};
