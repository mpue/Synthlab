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
#include "Project/Project.h"

using juce::Viewport;
using juce::Graphics;
using juce::ResizableWindow;

//==============================================================================
EditorComponent::EditorComponent(float sampleRate, int bufferSize) : resizerBar (&stretchableManager, 1, false)
{
    this->bufferSize = bufferSize;
    this->sampleRate = sampleRate;

    topTab = new MainTabbedComponent();
    bottomTab = new MainTabbedComponent();
    
    editor = new SynthEditor(sampleRate,bufferSize);
    editorView = new Viewport();
    
    editorView->setSize(500,200);
    editorView->setViewedComponent(editor);
    editorView->setScrollBarsShown(true,true);
    editorView->setScrollOnDragEnabled(false);
    editorView->setWantsKeyboardFocus(false);
    editorView->setMouseClickGrabsKeyboardFocus(false);
    
    topTab->addTab("Editor", juce::Colours::grey, editorView, false);
    
    
    mixerPanel = new MixerPanel();
    mixer = Mixer::getInstance();
    mixerPanel->setMixer(mixer);
    
    mixerView = new Viewport();
    mixerView->setViewedComponent(mixerPanel);
    mixerView->setScrollBarsShown(true,true);
    mixerView->setScrollOnDragEnabled(false);
    mixerView->setWantsKeyboardFocus(false);
    mixerView->setMouseClickGrabsKeyboardFocus(false);
    
    editor->setMixer(mixerPanel);
    editor->addChangeListener(this);
    
    bottomTab->addTab("Mixer", Colours::darkgrey, mixerView, false);
    
    addAndMakeVisible(topTab);
    addAndMakeVisible(resizerBar);
    addAndMakeVisible(bottomTab);
    
    stretchableManager.setItemLayout (0,            // for the properties
                                      -0.1, -0.9,   // must be between 50 pixels and 90% of the available space
                                      -0.7);        // and its preferred size is 30% of the total available space
    
    stretchableManager.setItemLayout (1,            // for the resize bar
                                      5, 5, 5);     // hard limit to 5 pixels
    
    stretchableManager.setItemLayout (2,            // for the main editor
                                      -0.1, -0.9,   // size must be between 50 pixels and 90% of the available space
                                      -0.3);

    Project::getInstance()->setSupplemental(this->bottomTab);
 
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
    delete editor;
    delete editorView;
    delete mixerPanel;
    delete mixerView;    
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
    juce::Component* comps[] = { topTab, &resizerBar, bottomTab };
    
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
    return navigator;
}

MixerPanel* EditorComponent::getMixerPanel() {
    return mixerPanel;
}

SynthEditor* EditorComponent::getEditor() {
    return editor;
}

MainTabbedComponent* EditorComponent::getEditorTab() {
    return topTab;
}

void EditorComponent::OpenTrackView()
{
    if (navigator == nullptr) {        
        navigator = new TrackNavigator(AudioManager::getInstance()->getDeviceManager());
        PrefabFactory::getInstance()->init(navigator);
        addMouseListener(navigator, true);
        addKeyListener(navigator);
        trackView = new Viewport();

        trackView->setSize(500, 200);
        trackView->setViewedComponent(navigator);
        trackView->setScrollBarsShown(true, true);
        trackView->setScrollOnDragEnabled(false);
        trackView->setWantsKeyboardFocus(false);
        trackView->setMouseClickGrabsKeyboardFocus(false);

    }
    topTab->addTab("Navigator", juce::Colours::grey, trackView, false);

}

void EditorComponent::changeListenerCallback(juce::ChangeBroadcaster *source) {
    
}
