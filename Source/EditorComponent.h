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
#include "Tracks/TrackNavigator.h"
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
    void ImportAudio();
    SynthEditor* getEditor();
    MixerPanel* getMixerPanel();
    Mixer* getMixer();
    TrackNavigator* getNavigator();
    MainTabbedComponent* getEditorTab();
    void OpenTrackView();
    virtual void changeListenerCallback (juce::ChangeBroadcaster* source) override;
private:
    
    int bufferSize;
    float sampleRate;

    SynthEditor* editor = nullptr;
    MixerPanel* mixerPanel = nullptr;
    juce::Viewport* editorView = nullptr;
    juce::Viewport* mixerView = nullptr;
    juce::Viewport* trackView = nullptr;
    Mixer* mixer = nullptr;
    
    MainTabbedComponent* topTab = nullptr;
    MainTabbedComponent* bottomTab = nullptr;

    juce::StretchableLayoutManager stretchableManager;
    juce::StretchableLayoutResizerBar resizerBar;

    TrackNavigator* navigator = nullptr;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EditorComponent)
};
