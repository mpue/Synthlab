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
#include "SequenceEditor.h"

//==============================================================================
/*
 */
class StepSequencerModule    : public Module
{
public:
    StepSequencerModule(float sampleRate,int buffersize);
    ~StepSequencerModule();
    
    
    void paint (juce::Graphics& g) override;
    virtual void process() override;
    virtual void configurePins() override;
    void eventReceived(Event *e) override;
    virtual juce::String getCategory() override {
        return "MIDI";
    }
    
    SequenceEditor* getEditor();

    
private:
    
    float tempo = 120;
    SequenceEditor* editor = nullptr;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StepSequencerModule)
};

