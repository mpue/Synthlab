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
    virtual ~RecordListener() {};
    
    typedef enum State {
        RECORDING_STARTED,
        RECORDING_STOPPED,
        PLAYING_STARTED,
        PLAYING_STOPPED
    } TState;
    
    typedef struct TStateChange {
        juce::String message = "";
        State newState;
    } StateChange;
    
    virtual void incomingMessage(MidiMessage* message) = 0;
    
    virtual void changedState(StateChange* change) = 0;

    
};

