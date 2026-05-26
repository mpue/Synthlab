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
EditorComponent::EditorComponent(float sampleRate, int bufferSize, PropertyView* properties, CommandParser& parser, TransportDisplayComponent* transport) : parser(parser)
{
    this->bufferSize = bufferSize;
    this->sampleRate = sampleRate;
    this->propertyView = properties;
	this->transportDisplay = transport;

    topTab = std::make_unique<MainTabbedComponent>();

    editor = std::make_unique<SynthEditor>(sampleRate,bufferSize);
	editorView = std::make_unique<Viewport>();

    editorView->setSize(500,200);
    editorView->setViewedComponent(editor.get(),false);
    editorView->setScrollBarsShown(true,true);
    editorView->setScrollOnDragEnabled(false);
    editorView->setWantsKeyboardFocus(false);
    editorView->setMouseClickGrabsKeyboardFocus(false);

    editor->setWantsKeyboardFocus(true);
    editor->setMouseClickGrabsKeyboardFocus(true);

    // SynthEditor als KeyListener fuer das Top-Level-Component registrieren —
    // zum Zeitpunkt dieses Aufrufs hat EditorComponent allerdings noch keinen
    // Parent, getTopLevelComponent() liefert `this`. Das ist ein No-Op.
    // MainComponent::addKeyListener(this) deckt die globalen Shortcuts schon
    // ab, hier daher belassen / spaeter aufraeumen.
    if (auto* topLevel = getTopLevelComponent())
        topLevel->addKeyListener(editor.get());

    setMouseClickGrabsKeyboardFocus(true);

    topTab->addTab("Editor", juce::Colours::grey, editorView.get(), false);
    topTab->setInterceptsMouseClicks(true, true);

    // MixerPanel wird hier nur als Lifetime-Anker erzeugt — der eigentliche
    // Inhalt landet in MainComponent's Mixer-DockablePanel ueber den von dort
    // gerufenen getMixerPanel(). MainComponent's MixerDockContent setzt sich
    // selbst als Parent des MixerPanel; addAndMakeVisible hier entfaellt.
    mixerPanel = std::make_shared<MixerPanel>();

    editor->setMixer(mixerPanel);
	editor->setMouseClickGrabsKeyboardFocus(true);
    editor->setWantsKeyboardFocus(true);

    addAndMakeVisible(topTab.get());

    // bottomTab / Project::setSupplemental setzt MainComponent jetzt selber,
    // weil der supplementalTab in einem eigenen Tools-DockablePanel lebt.

    OpenTrackView();

    setupKeyboardFocus();
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
    // setSupplemental wird von ToolsDockContent (MainComponent) gestellt /
    // genullt — hier nichts mehr zu tun.
    editorView = nullptr;
    mixerPanel = nullptr;
    navigator = nullptr;
    trackView = nullptr;
    sequence = nullptr;
}

void EditorComponent::paint (Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));   // clear the background
    g.setColour (Colours::grey);
}

void EditorComponent::resized()
{
    if (topTab != nullptr)
        topTab->setBounds (getLocalBounds());
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

// openConsole / openLog / getBottomTab wurden mit dem Bottom-Tab-Refactor
// entfernt. Console-/Log-Tabs leben jetzt als eigene DockablePanels in
// MainComponent (Console als Tab im Tools-Panel).

void EditorComponent::OpenTrackView()
{
    if (navigator == nullptr) {        
        trackView = std::make_unique<Viewport>();
        navigator = std::make_shared<TrackNavigator>(AudioManager::getInstance()->getDeviceManager(), trackView.get(), propertyView, transportDisplay);
        
        PrefabFactory::getInstance()->init(navigator.get());
        // addMouseListener(navigator.get(), true);
        editor->setNavigator(navigator.get());
        navigator->setEditor(editor.get());
        trackView->setSize(500, 200);
        trackView->setViewedComponent(navigator.get());
        trackView->setScrollBarsShown(true, true);
        trackView->setScrollOnDragEnabled(false);
        trackView->setInterceptsMouseClicks(true, true);
	    
        trackView->addMouseListener(this, true);

	    getTopLevelComponent()->addKeyListener(navigator.get());
        navigator->setMouseClickGrabsKeyboardFocus(true);
        navigator->setWantsKeyboardFocus(true);
        navigator->setInterceptsMouseClicks(true, true);
        navigator->toFront(false);

        // Default-Setup: 2 Audio-Tracks fuer leeres Projekt. Wenn nachher ein
        // File geladen wird, leert der Load-Pfad clearTracks() vorher und
        // ersetzt sie mit den File-eigenen Tracks.
        // navigator->ensureDefaultAudioTracks(2);
    }
    // Only add the Navigator tab once
    bool navigatorTabExists = false;
    for (int i = 0; i < topTab->getNumTabs(); i++) {
        if (topTab->getTabNames()[i] == "Navigator") {
            navigatorTabExists = true;
            topTab->setCurrentTabIndex(i);
            break;
        }
    }
    if (!navigatorTabExists) {
        topTab->addTab("Navigator", juce::Colours::grey, trackView.get(), false);
        topTab->setCurrentTabIndex(topTab->getNumTabs() - 1);
    }
    navigator->setInterceptsMouseClicks(true, true);
    navigator->toFront(true);
}

void EditorComponent::changeListenerCallback(juce::ChangeBroadcaster *source) {
    
}

void EditorComponent::setupKeyboardFocus()
{

}
