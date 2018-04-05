/*
  ==============================================================================

    MidiGate.h
    Created: 4 Apr 2018 3:48:23pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"

//==============================================================================
/*
*/
class MidiOut    : public Module
{
public:
    MidiOut();
    ~MidiOut();

    void paint (Graphics& g) override;

    void message(MidiMessage* message);
    void setNote(int note);
    void gate(bool on, int velocity);
    void setDeviceManager(AudioDeviceManager* manager);
    
private:
    AudioDeviceManager* deviceManager;
    int note = 0;
    int velocity = 0;
    bool gateOn = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiOut)
};
