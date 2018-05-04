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
class EditorComponent    : public juce::Component, public juce::ChangeListener
{
public:
    EditorComponent(float sampleRate, int bufferSize);
    ~EditorComponent();

    void paint (juce::Graphics&) override;
    void resized() override;

    SynthEditor* getEditor();
    MixerPanel* getMixerPanel();
    Mixer* getMixer();
    
    MainTabbedComponent* getEditorTab();
    
    virtual void changeListenerCallback (juce::ChangeBroadcaster* source) override;
private:
    
    int bufferSize;
    float sampleRate;

    SynthEditor* editor = nullptr;
    MixerPanel* mixerPanel = nullptr;
    juce::Viewport* editorView = nullptr;
    juce::Viewport* mixerView = nullptr;
    Mixer* mixer = nullptr;
    
    MainTabbedComponent* topTab = nullptr;
    MainTabbedComponent* bottomTab = nullptr;

    juce::StretchableLayoutManager stretchableManager;
    juce::StretchableLayoutResizerBar resizerBar;

    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EditorComponent)
};
