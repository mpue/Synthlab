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

using juce::Viewport;
using juce::Graphics;
using juce::ResizableWindow;

//==============================================================================
EditorComponent::EditorComponent(float sampleRate, int bufferSize, PropertyView* properties, CommandParser& parser) : resizerBar (&stretchableManager, 1, false)
{
    this->bufferSize = bufferSize;
    this->sampleRate = sampleRate;
    this->propertyView = properties;

    topTab = std::make_unique<MainTabbedComponent>();
    bottomTab = std::make_unique<MainTabbedComponent>();
    
    editor = std::make_unique<SynthEditor>(sampleRate,bufferSize);
	editorView = std::make_unique<Viewport>();
    
    editorView->setSize(500,200);
    editorView->setViewedComponent(editor.get());
    editorView->setScrollBarsShown(true,true);
    editorView->setScrollOnDragEnabled(false);
    editorView->setWantsKeyboardFocus(false);
    editorView->setMouseClickGrabsKeyboardFocus(false);
    
    topTab->addTab("Editor", juce::Colours::grey, editorView.get(), false);
        
    mixerPanel = std::make_unique<MixerPanel>();
    mixer = Mixer::getInstance();
    mixerPanel->setMixer(mixer);
    
    mixerView = std::make_unique<Viewport>();
    mixerView->setViewedComponent(mixerPanel.get());
    mixerView->setScrollBarsShown(true,true);
    mixerView->setScrollOnDragEnabled(false);
    mixerView->setWantsKeyboardFocus(false);
    mixerView->setMouseClickGrabsKeyboardFocus(false);
    
    editor->setMixer(mixerPanel.get());
    editor->addChangeListener(this);
    
    bottomTab->addTab("Mixer", Colours::darkgrey, mixerView.get(), false);

    console = std::make_unique<ShellConsoleComponent>(parser);
    bottomTab->addTab("Console", Colours::darkgrey, console.get(), false);

	sequence = std::make_unique<juce::MidiMessageSequence>();
	pianoRoll = std::make_unique<PianoRollComponent>(*sequence);

	pianoView = std::make_unique<Viewport>();

    pianoView->setViewedComponent(pianoRoll.get());
    pianoView->setScrollBarsShown(true, true);
    pianoView->setScrollOnDragEnabled(false);
    pianoView->setWantsKeyboardFocus(false);
    pianoView->setMouseClickGrabsKeyboardFocus(false);
    
    pianoRoll->setSize(2000, 800);
	pianoRoll->setGridResolution(16);

	bottomTab->addTab("Piano Roll", Colours::darkgrey, pianoView.get(), false);

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

EditorComponent::~EditorComponent()
{
    editor = nullptr;
    editorView = nullptr;
    mixerPanel = nullptr;
    mixerView =  nullptr;    
    navigator = nullptr;
    trackView = nullptr;
    console = nullptr;
	pianoRoll = nullptr;
	pianoView = nullptr;
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

Mixer* EditorComponent::getMixer() {
    return mixer;
}

TrackNavigator* EditorComponent::getNavigator()
{
    return navigator.get();
}

MixerPanel* EditorComponent::getMixerPanel() {
    return mixerPanel.get();
}

SynthEditor* EditorComponent::getEditor() {
    return editor.get();
}

MainTabbedComponent* EditorComponent::getEditorTab() {
    return topTab.get();
}

void EditorComponent::OpenTrackView()
{
    if (navigator == nullptr) {        
        trackView = std::make_unique<Viewport>();
        navigator = std::make_unique<TrackNavigator>(AudioManager::getInstance()->getDeviceManager(), trackView.get(), propertyView);
        
        PrefabFactory::getInstance()->init(navigator.get());
        addMouseListener(navigator.get(), true);
        editor->setNavigator(navigator.get());
        trackView->setSize(500, 200);
        trackView->setViewedComponent(navigator.get());
        trackView->setScrollBarsShown(true, true);
        trackView->setScrollOnDragEnabled(false);
        trackView->setWantsKeyboardFocus(false);
        trackView->setMouseClickGrabsKeyboardFocus(false);
		getTopLevelComponent()->addKeyListener(navigator.get());
        navigator->setMouseClickGrabsKeyboardFocus(true);
        navigator->setWantsKeyboardFocus(true);
        
    }
    topTab->addTab("Navigator", juce::Colours::grey, trackView.get(), false);
    topTab->setCurrentTabIndex(1);


}

void EditorComponent::changeListenerCallback(juce::ChangeBroadcaster *source) {
    
}
