/*
  ==============================================================================

    RecordListener.h
    Created: 7 Apr 2018 6:08:18pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

class RecordListener {

public:
    typedef enum State {
        RECORDING_STARTED,
        RECORDING_STOPPED,
        PLAYING_STARTED,
        PLAYING_STOPPED
    };
    
    typedef struct TStateChange {
        String message = "";
        State newState;
    } StateChange;
    
    virtual void incomingMessage(MidiMessage* message) = 0;
    
    virtual void changedState(StateChange* change) = 0;

    
};

