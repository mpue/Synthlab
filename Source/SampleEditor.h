#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "AudioThumbnailComponent.h"
#include "SamplerModule.h"


class SampleEditor  : public Component, public MidiKeyboardStateListener, public MouseListener
{
public:

    SampleEditor (int buffersize, float sampleRate, AudioFormatManager* manager, SamplerModule* module);
    ~SampleEditor();

    void paint (Graphics& g) override;
    void resized() override;

    virtual void handleNoteOff (MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity) override;
    virtual void handleNoteOn (MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity) override;
    
    void mouseDoubleClick (const MouseEvent& event) override;
    
    void openSample(SamplerModule* sm);
    

    
private:
    MidiKeyboardState state;
    MidiKeyboardComponent::Orientation orientation = MidiKeyboardComponent::Orientation::horizontalKeyboard;

    int buffersize;

    ScopedPointer<MidiKeyboardComponent> midiKeyboard;
    ScopedPointer<AudioThumbnailComponent> component;

    SamplerModule* sampleModule;
    
    

    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleEditor)
};


