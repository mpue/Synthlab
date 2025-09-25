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
    
    typedef struct TRawEvent {
        Event* gate = nullptr;
        Event* note = nullptr;
    } RawEvent;

    RawEvent* raw = nullptr;
    
    void paint (juce::Graphics& g) override;

    void message(juce::MidiMessage* message);
    void setNote(int note);
    void gate(bool on, int note, int velocity);
    void setDeviceManager(juce::AudioDeviceManager* manager);
    void eventReceived(Event* e) override;
    virtual void configurePins() override;
    
    virtual juce::String getCategory() override {
        return "MIDI";
    }
    
private:
    juce::AudioDeviceManager* deviceManager;
    int note = 0;
    int velocity = 0;
    bool gateOn = false;
    bool eventComplete = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiOut)
};
