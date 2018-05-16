/*
  ==============================================================================

    Topic.h
    Created: 16 May 2018 8:52:28pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once


#include "../JuceLibraryCode/JuceHeader.h"

class Topic {
  
public:
    Topic();
    ~Topic();
    
    void setName(String name);
    String getName();
    
    float getValue();
    void setValue(float value);
    
private:
    String name;
    float value;
    
};
