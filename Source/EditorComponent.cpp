/*
  ==============================================================================

    EditorComponent.cpp
    Created: 24 Apr 2018 3:06:51pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditorComponent.h"
#include "SynthEditor.h"
#include "Mixer.h"
#include "Actions/AddModuleAction.h"
#include "Project/Project.h"
#include "ComponentTreeDebugger.h"

using juce::Viewport;
using juce::Graphics;
using juce::ResizableWindow;

//==============================================================================
EditorComponent::EditorComponent(float sampleRate, int bufferSize, PropertyView* properties, CommandParser& parser, TransportDisplayComponent* transport) : resizerBar(&stretchableManager, 1, false), parser(parser)
{
    this->bufferSize = bufferSize;
    this->sampleRate = sampleRate;
    this->propertyView = properties;
	this->transportDisplay = transport;

    topTab = std::make_unique<MainTabbedComponent>();
    bottomTab = std::make_unique<MainTabbedComponent>();
    
    editor = std::make_unique<SynthEditor>(sampleRate,bufferSize);
	editorView = std::make_unique<Viewport>();
    
    editorView->setSize(500,200);
    editorView->setViewedComponent(editor.get(),false);
    editorView->setScrollBarsShown(true,true);
    editorView->setScrollOnDragEnabled(false);
    editorView->setWantsKeyboardFocus(false);
    editorView->setMouseClickGrabsKeyboardFocus(false);
    
    topTab->addTab("Editor", juce::Colours::grey, editorView.get(), false);
        
    mixerPanel = std::make_shared<MixerPanel>();
    
    
    mixerView = std::make_unique<Viewport>();
    mixerView->setViewedComponent(mixerPanel.get(),false);
    mixerView->setScrollBarsShown(true,true);
    mixerView->setScrollOnDragEnabled(false);
    mixerView->setWantsKeyboardFocus(false);
    mixerView->setMouseClickGrabsKeyboardFocus(false);
    
    editor->setMixer(mixerPanel);
    editor->addChangeListener(this);	
	editor->setInterceptsMouseClicks(true, true);
	editor->setMouseClickGrabsKeyboardFocus(true);
    bottomTab->addTab("Mixer", Colours::darkgrey, mixerView.get(), false);

    addAndMakeVisible(topTab.get());
    addAndMakeVisible(resizerBar);
    addAndMakeVisible(bottomTab.get());
    
    stretchableManager.setItemLayout (0,            // for the properties
                                      -0.1, -0.9,   // must be between 50 pixels and 90% of the available space
                                      -0.7);        // and its preferred size is 30% of the total available space
    
    stretchableManager.setItemLayout (1,            // for the resize bar
                                      5, 5, 5);     // hard limit to 5 pixels
    
    stretchableManager.setItemLayout (2,            // for the main editor
                                      -0.1, -0.9,   // size must be between 50 pixels and 90% of the available space
                                      -0.3);

    Project::getInstance()->setSupplemental(this->bottomTab.get());

    OpenTrackView();

 //   auto* treeDebugger = new ComponentTreeDebugger(getTopLevelComponent());
 //   treeDebugger->setSize(300, 600);
 //   treeDebugger->setName("Komponenten-Baum");
 //   treeDebugger->setAlwaysOnTop(true);
 //   treeDebugger->setVisible(true);

	//topTab->addTab("Debugger", Colours::darkgrey, treeDebugger, false);
    


}


void EditorComponent::ImportAudio() {

    FileChooser chooser("Select a file to add...");

    if (chooser.browseForFileToOpen()) {
        File file = chooser.getResult();

        if (file.getFileExtension() == ".wav" || file.getFileExtension() == ".aif" || file.getFileExtension() == ".mp3") {
            navigator->getCurrentTrack()->addRegion(file.getFileNameWithoutExtension(), file, this->sampleRate);
            Project::getInstance()->addAudioFile(file.getFileNameWithoutExtension(), file.getFullPathName());
            navigator->repaint();
        }

    }
}

void EditorComponent::ImportMidi() {
    // Check if we have a selected track
    if (navigator == nullptr || navigator->getCurrentTrack() == nullptr) {
        juce::AlertWindow::showMessageBox(juce::AlertWindow::WarningIcon,
            "No Track Selected",
            "Please select a MIDI or Instrument track before importing MIDI files.");
        return;
    }

    Track* currentTrack = navigator->getCurrentTrack();
    
    // Check if the selected track is MIDI or Instrument type
    if (currentTrack->getType() != Track::MIDI && currentTrack->getType() != Track::INSTRUMENT) {
        juce::AlertWindow::showMessageBox(juce::AlertWindow::WarningIcon,
            "Invalid Track Type",
            "MIDI files can only be imported to MIDI or Instrument tracks.");
        return;
    }

    FileChooser chooser("Select MIDI file to import...",
                       File::getSpecialLocation(File::userDocumentsDirectory),
                       "*.mid;*.midi");

    if (chooser.browseForFileToOpen()) {
        File file = chooser.getResult();
        
        if (file.getFileExtension().toLowerCase() == ".mid" || 
            file.getFileExtension().toLowerCase() == ".midi") {
            
            // Load MIDI file
            juce::MidiFile midiFile;
            juce::FileInputStream fileStream(file);
            
            if (midiFile.readFrom(fileStream)) {
                // Convert MIDI file to our format
                importMidiFileToTrack(midiFile, currentTrack);
                navigator->repaint();
                
                juce::Logger::getCurrentLogger()->writeToLog(
                    "Successfully imported MIDI file: " + file.getFileName());
            } else {
                juce::AlertWindow::showMessageBox(juce::AlertWindow::WarningIcon,
                    "Import Error",
                    "Failed to read MIDI file: " + file.getFileName());
            }
        } else {
            juce::AlertWindow::showMessageBox(juce::AlertWindow::WarningIcon,
                "Invalid File Type",
                "Please select a valid MIDI file (.mid or .midi)");
        }
    }
}

void EditorComponent::importMidiFileToTrack(const juce::MidiFile& midiFile, Track* track) {
    if (!track || midiFile.getNumTracks() == 0) {
        return;
    }

    // Get current playback position or start of timeline
    long startPosition = navigator->getSamplePosition();
    
    // Create a new MIDI region - estimate length from MIDI file
    double midiLengthInSeconds = midiFile.getLastTimestamp();
    long regionLengthInSamples = (long)(midiLengthInSeconds * this->sampleRate);
    
    // Make sure the region is at least 1 second long
    if (regionLengthInSamples < this->sampleRate) {
        regionLengthInSamples = this->sampleRate;
    }
    
    MidiRegion* region = track->addMidiRegion(this->sampleRate, startPosition, regionLengthInSamples);
    
    if (!region) {
        juce::Logger::getCurrentLogger()->writeToLog("Failed to create MIDI region");
        return;
    }

    // Process all tracks in the MIDI file
    for (int trackIndex = 0; trackIndex < midiFile.getNumTracks(); ++trackIndex) {
        const juce::MidiMessageSequence* sequence = midiFile.getTrack(trackIndex);
        
        if (!sequence) continue;
        
        // Convert time format from ticks to samples
        double ticksPerSecond = midiFile.getTimeFormat();
        if (midiFile.getTimeFormat() < 0) {
            // SMPTE time format
            int framesPerSecond = -(midiFile.getTimeFormat() >> 8);
            int ticksPerFrame = midiFile.getTimeFormat() & 0xFF;
            ticksPerSecond = framesPerSecond * ticksPerFrame;
        } else {
            // Ticks per quarter note
            double bpm = 120.0; // Default BPM, could be extracted from MIDI file
            ticksPerSecond = (bpm / 60.0) * midiFile.getTimeFormat();
        }
        
        // Add all MIDI messages from this track
        for (int i = 0; i < sequence->getNumEvents(); ++i) {
            juce::MidiMessageSequence::MidiEventHolder* event = sequence->getEventPointer(i);
            if (!event) continue;
            
            const juce::MidiMessage& message = event->message;
            
            // Convert timestamp from ticks to samples
            double timeInSeconds = message.getTimeStamp() / ticksPerSecond;
            int sampleOffset = (int)(timeInSeconds * this->sampleRate);
            
            // Filter out unwanted message types (keep notes, controllers, pitch bend)
            if (message.isNoteOnOrOff() || message.isController() || message.isPitchWheel()) {
                // Create a copy of the message with the correct channel
                juce::MidiMessage* newMessage = nullptr;
                
                if (message.isNoteOn()) {
                    newMessage = new juce::MidiMessage(juce::MidiMessage::noteOn(track->getMidiChannel(), 
                        message.getNoteNumber(), message.getVelocity()));
                } else if (message.isNoteOff()) {
                    newMessage = new juce::MidiMessage(juce::MidiMessage::noteOff(track->getMidiChannel(), 
                        message.getNoteNumber(), message.getVelocity()));
                } else if (message.isController()) {
                    newMessage = new juce::MidiMessage(juce::MidiMessage::controllerEvent(track->getMidiChannel(), 
                        message.getControllerNumber(), message.getControllerValue()));
                } else if (message.isPitchWheel()) {
                    newMessage = new juce::MidiMessage(juce::MidiMessage::pitchWheel(track->getMidiChannel(), 
                        message.getPitchWheelValue()));
                } else {
                    // Fallback - copy original message
                    newMessage = new juce::MidiMessage(message);
                }
                
                newMessage->setTimeStamp(sampleOffset);
                
                // Add to region
                region->addMessage(newMessage, timeInSeconds, sampleOffset);
            }
        }
    }
    
    // Update the region's display
    region->rebuildThumbnail();
    region->updateThumb();
    region->repaint();
    
    juce::Logger::getCurrentLogger()->writeToLog(
        "Imported MIDI file with " + juce::String(midiFile.getNumTracks()) + " tracks");
}

EditorComponent::~EditorComponent()
{
    editor->setTab(nullptr);
    editor = nullptr;
    Project::getInstance()->setSupplemental(nullptr); 
    editorView = nullptr;
    mixerPanel = nullptr;
    mixerView =  nullptr;    
    navigator = nullptr;
    trackView = nullptr;
    console = nullptr;
	sequence = nullptr;
}

void EditorComponent::paint (Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));   // clear the background
    g.setColour (Colours::grey);
}

void EditorComponent::resized()
{
    mixerPanel->resized();
    auto r = getLocalBounds();
    
    // make a list of two of our child components that we want to reposition
    juce::Component* comps[] = { topTab.get(), &resizerBar, bottomTab.get()};
    
    // this will position the 3 components, one above the other, to fit
    // vertically into the rectangle provided.
    stretchableManager.layOutComponents (comps, 3,
                                         r.getX(), r.getY(), r.getWidth(), r.getHeight(),
                                         true, true);
}

TrackNavigator* EditorComponent::getNavigator()
{
    return navigator.get();
}

std::shared_ptr<MixerPanel>EditorComponent::getMixerPanel() {
    return mixerPanel;
}

SynthEditor* EditorComponent::getEditor() {
    return editor.get();
}

MainTabbedComponent* EditorComponent::getEditorTab() {
    return topTab.get();
}

void EditorComponent::openConsole() {
    console = std::make_unique<ShellConsoleComponent>(parser);
    bottomTab->addTab("Console", Colours::darkgrey, console.get(), false);
}

void EditorComponent::OpenTrackView()
{
    if (navigator == nullptr) {        
        trackView = std::make_unique<Viewport>();
        navigator = std::make_unique<TrackNavigator>(AudioManager::getInstance()->getDeviceManager(), trackView.get(), propertyView, transportDisplay);
        
        PrefabFactory::getInstance()->init(navigator.get());
        addMouseListener(navigator.get(), true);
        editor->setNavigator(navigator.get());
        trackView->setSize(500, 200);
        trackView->setViewedComponent(navigator.get());
        trackView->setScrollBarsShown(true, true);
        trackView->setScrollOnDragEnabled(false);
	        
	    getTopLevelComponent()->addKeyListener(navigator.get());

        navigator->setMouseClickGrabsKeyboardFocus(true);
        navigator->setWantsKeyboardFocus(true);
        navigator->setInterceptsMouseClicks(true, true);

        
    }
    topTab->addTab("Navigator", juce::Colours::grey, trackView.get(), false);
    topTab->setCurrentTabIndex(1);


}

void EditorComponent::changeListenerCallback(juce::ChangeBroadcaster *source) {
    
}
