#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "AudioThumbnailComponent.h"
#include "SamplerModule.h"
#include "SamplePropertiesPanel.h"

class SampleEditor  : public Component, public MidiKeyboardStateListener, public MidiInputCallback
{
public:

    SampleEditor (int buffersize, float sampleRate, AudioFormatManager* manager, SamplerModule* module);
    ~SampleEditor();

    void paint (Graphics& g) override;
    void resized() override;

    virtual void handleNoteOff (MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity) override;
    virtual void handleNoteOn (MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity) override;
    
    
    void handleIncomingMidiMessage (MidiInput* source, const MidiMessage& message) override;
    
private:
    MidiKeyboardState state;
    MidiKeyboardComponent::Orientation orientation = MidiKeyboardComponent::Orientation::horizontalKeyboard;

    int buffersize;

    ScopedPointer<MidiKeyboardComponent> midiKeyboard;
    AudioThumbnailComponent* component;
    ScopedPointer<SamplePropertiesPanel> propertiesPanel;
    
    SamplerModule* sampleModule;
    
    

    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleEditor)
};


