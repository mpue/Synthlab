/*
  ==============================================================================

    Pin.h
    Created: 1 Apr 2018 6:49:53pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once

class Pin {


public:

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
    bool selected;
	Direction direction;
    Type type;
    
};
