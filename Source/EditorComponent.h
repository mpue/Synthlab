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
class EditorComponent    : public Component, public ChangeListener
{
public:
    EditorComponent(float sampleRate, int bufferSize);
    ~EditorComponent();

    void paint (Graphics&) override;
    void resized() override;

    SynthEditor* getEditor();
    MixerPanel* getMixerPanel();
    Mixer* getMixer();
    
    virtual void changeListenerCallback (ChangeBroadcaster* source) override;
private:
    
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
