/*
  ==============================================================================

    ISampleEditor.h
    Created: 20 Apr 2018 3:43:39pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

class ISampleEditor : public Component {
public:
    virtual void openSample() = 0; 
};
