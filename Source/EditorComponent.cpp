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
#include "Project.h"

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
    mixer = new Mixer();
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

EditorComponent::~EditorComponent()
{
    delete mixer;
    delete mixerPanel;
    delete mixerView;
    delete editor;
    delete editorView;
    delete topTab;
    delete bottomTab;

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
    Component* comps[] = { topTab, &resizerBar, bottomTab };
    
    // this will position the 3 components, one above the other, to fit
    // vertically into the rectangle provided.
    stretchableManager.layOutComponents (comps, 3,
                                         r.getX(), r.getY(), r.getWidth(), r.getHeight(),
                                         true, true);
}

Mixer* EditorComponent::getMixer() {
    return mixer;
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

void EditorComponent::changeListenerCallback(juce::ChangeBroadcaster *source) {
    
}
