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

    int x;
    int y;
    bool selected;
	Direction direction;

    
};
