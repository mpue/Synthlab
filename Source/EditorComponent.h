/*
  ==============================================================================

    EditorComponent.h
    Created: 24 Apr 2018 3:06:51pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "SynthEditor.h"
#include "MainTabbedComponent.h"
#include "MixerPanel.h"
#include "Mixer.h"


//==============================================================================
/*
*/
class EditorComponent    : public Component
{
public:
    EditorComponent(float sampleRate, int bufferSize);
    ~EditorComponent();

    void paint (Graphics&) override;
    void resized() override;

    SynthEditor* getEditor();
    MixerPanel* getMixerPanel();
    Mixer* getMixer();
    
private:

    void initChannels();
    int getNumActiveChannels(int i) {
        i = i - ((i >> 1) & 0x55555555);
        i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
        return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
    }
    
    int bufferSize;
    float sampleRate;

    SynthEditor* editor = nullptr;
    MixerPanel* mixerPanel = nullptr;
    Viewport* editorView = nullptr;
    Viewport* mixerView = nullptr;
    Mixer* mixer = nullptr;
    
    MainTabbedComponent* topTab = nullptr;
    MainTabbedComponent* bottomTab = nullptr;

    StretchableLayoutManager stretchableManager;
    StretchableLayoutResizerBar resizerBar;

    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EditorComponent)
};
