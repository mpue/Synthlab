/*
  ==============================================================================

    BusListener.h
    Created: 16 May 2018 9:06:12pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once

class BusListener {

public:
    virtual ~BusListener() {}; 
    virtual void topicChanged(Topic* t) = 0;
    
};
