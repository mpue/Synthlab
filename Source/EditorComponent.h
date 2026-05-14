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
#include "ShellConsoleComponent.h"
#include "LogView.h"
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
    MainTabbedComponent* getBottomTab();
    void OpenTrackView();
	void openConsole();
	// Re-adds the LogView tab to the bottomTab if missing, otherwise just
	// focuses it. Used by the View > Log menu to recover the log after a
	// project open dropped it.
	void openLog();
    virtual void changeListenerCallback (juce::ChangeBroadcaster* source) override;
    
    // Keyboard-Focus-Setup nach der vollst�ndigen Initialisierung
    void setupKeyboardFocus();

private:
    
    void importMidiFileToTrack(const juce::MidiFile& midiFile, Track* track);
    
    int bufferSize;
    float sampleRate;

	CommandParser& parser;

    std::unique_ptr<SynthEditor> editor = nullptr;
    std::shared_ptr<MixerPanel> mixerPanel = nullptr;
    std::unique_ptr<juce::Viewport> editorView = nullptr;
    std::unique_ptr<juce::Viewport> mixerView = nullptr;
    std::unique_ptr<juce::Viewport> trackView = nullptr;
    
    std::unique_ptr<MainTabbedComponent> topTab = nullptr;
    std::unique_ptr<MainTabbedComponent> bottomTab = nullptr;

    juce::StretchableLayoutManager stretchableManager;
    juce::StretchableLayoutResizerBar resizerBar;

    std::shared_ptr<TrackNavigator> navigator = nullptr;
    PropertyView* propertyView = nullptr;
    
    std::unique_ptr<ShellConsoleComponent> console = nullptr;
    std::unique_ptr<LogView> logView = nullptr;

    std::unique_ptr<juce::MidiMessageSequence> sequence = nullptr;

	TransportDisplayComponent* transportDisplay = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EditorComponent)
};
