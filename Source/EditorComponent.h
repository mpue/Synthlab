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
#include "PropertyView.h"
// ShellConsoleComponent / LogView werden jetzt von MainComponent als
// DockablePanel-Content verwaltet — hier nicht mehr referenziert.
#include "CommandParser.h"
#include "PianoRollComponent.h"
#include "TransportDisplayComponent.h"
//==============================================================================
/*
*/
class EditorComponent    : public juce::Component, public juce::ChangeListener
{
public:
    EditorComponent(float sampleRate, int bufferSize, PropertyView* properties, CommandParser& parser, TransportDisplayComponent* transport);
    ~EditorComponent();

    void paint (juce::Graphics&) override;
    void resized() override;
    void ImportAudio();
    void ImportMidi();
    SynthEditor* getEditor();
    std::shared_ptr<MixerPanel> getMixerPanel();
    TrackNavigator* getNavigator();
    MainTabbedComponent* getEditorTab();
    void OpenTrackView();
    virtual void changeListenerCallback (juce::ChangeBroadcaster* source) override;
    
    // Keyboard-Focus-Setup nach der vollst�ndigen Initialisierung
    void setupKeyboardFocus();

private:
    
    void importMidiFileToTrack(const juce::MidiFile& midiFile, Track* track);
    
    int bufferSize;
    float sampleRate;

	CommandParser& parser;

    std::unique_ptr<SynthEditor> editor = nullptr;
    // MixerPanel lebt jetzt im Mixer-DockablePanel (MainComponent). Der hier
    // gehaltene shared_ptr ist nur der Lifetime-Anker fuer SynthEditor::setMixer.
    std::shared_ptr<MixerPanel> mixerPanel = nullptr;
    std::unique_ptr<juce::Viewport> editorView = nullptr;
    std::unique_ptr<juce::Viewport> trackView = nullptr;

    std::unique_ptr<MainTabbedComponent> topTab = nullptr;
    // bottomTab/mixerView/console/logView sind raus — Mixer/Log/Level Meter/
    // Spektrum/Tools sind eigene DockablePanels (im MainComponent verwaltet).
    // stretchableManager + resizerBar raus, weil kein Top/Bottom-Split mehr.

    std::shared_ptr<TrackNavigator> navigator = nullptr;
    PropertyView* propertyView = nullptr;

    std::unique_ptr<juce::MidiMessageSequence> sequence = nullptr;

	TransportDisplayComponent* transportDisplay = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EditorComponent)
};
