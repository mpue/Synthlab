/*
  ==============================================================================

    VolumeAdjustable.h
    Created: 10 May 2018 10:51:15am
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once
#include "../JuceLibraryCode/JuceHeader.h"

class VolumeAdjustable : public ChangeBroadcaster {
public:
    
    virtual ~VolumeAdjustable() {
        removeAllChangeListeners();
    };
    
    virtual void setVolume(float volume) {
        this->volume = volume;
        sendChangeMessage();
    }
    
    virtual float getVolume() {
        return volume;
    }
protected:
    float volume = 0;
};
