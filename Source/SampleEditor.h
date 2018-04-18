#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "AudioThumbnailComponent.h"
#include "SamplerModule.h"

class SampleEditor  : public Component
{
public:

    SampleEditor (int buffersize, float sampleRate, AudioFormatManager* manager, SamplerModule* module);
    ~SampleEditor();

    void paint (Graphics& g) override;
    void resized() override;

private:
    MidiKeyboardState state;
    MidiKeyboardComponent::Orientation orientation = MidiKeyboardComponent::Orientation::horizontalKeyboard;

    int buffersize;

    ScopedPointer<MidiKeyboardComponent> midiKeyboard;
    ScopedPointer<AudioThumbnailComponent> component;

    SamplerModule* sampleModule;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleEditor)
};


