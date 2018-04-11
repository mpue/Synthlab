/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"
#include "SynthEditor.h"
#include "PropertyView.h"
#include "MainTabbedComponent.h"
#include "MidiGate.h"
#include "MidiNote.h"
#include "PrefabFactory.h"

//==============================================================================
MainComponent::MainComponent() : resizerBar (&stretchableManager, 1, true)
{
    // Make sure you set the size of the component after
    // you add any child components.
    setSize (1280, 800);

    // specify the number of input and output channels that we want to open
    setAudioChannels (2, 2);

    String userHome = File::getSpecialLocation(File::userHomeDirectory).getFullPathName();
    
    File appDir = File(userHome+"/.Synthlab");
    
    if (!appDir.exists()) {
        appDir.createDirectory();
    }
    
    File configFile = File(userHome+"/.Synthlab/config.xml");
    
    if (configFile.exists()) {
        ScopedPointer<XmlElement> xml = XmlDocument(configFile).getDocumentElement();
        deviceManager.initialise(2,2, xml, true);
    }
     
    propertyView =  new PropertyView();
    

    editor->addChangeListener(propertyView);
    addAndMakeVisible (propertyView);
    addAndMakeVisible (resizerBar);
    addAndMakeVisible (tab);
    
    // we have to set up our StretchableLayoutManager so it know the limits and preferred sizes of it's contents
    stretchableManager.setItemLayout (0,            // for the fileTree
                                      -0.1, -0.9,   // must be between 50 pixels and 90% of the available space
                                      -0.2);        // and its preferred size is 30% of the total available space
    
    stretchableManager.setItemLayout (1,            // for the resize bar
                                      5, 5, 5);     // hard limit to 5 pixels
    
    stretchableManager.setItemLayout (2,            // for the imagePreview
                                      -0.1, -0.9,   // size must be between 50 pixels and 90% of the available space
                                      -0.8);
    
    resized();
    

    
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
    
    editor->removeAllChangeListeners();
	delete editor;
    delete tab;
    delete propertyView;
    PrefabFactory::getInstance()->destroy();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    // This function will be called when the audio device is started, or when
    // its settings (i.e. sample rate, block size, etc) are changed.

    // You can use this function to initialise any resources you might need,
    // but be careful - it will be called on the audio thread, not the GUI thread.

    // For more details, see the help for AudioProcessor::prepareToPlay()
    // editor->setSamplerate(sampleRate);
    // editor->setBufferSize(samplesPerBlockExpected);
    
    editor = new SynthEditor(sampleRate,samplesPerBlockExpected);
    tab = new MainTabbedComponent();
    tab->setBounds(0,0,getWidth(),getHeight());
    
    
    view = new Viewport();
    
    addAndMakeVisible(view);
    
    view->setSize(500,200);
    view->setViewedComponent(editor);
    view->setScrollBarsShown(true,true);
    view->setScrollOnDragEnabled(true);
    view->setWantsKeyboardFocus(false);
    view->setMouseClickGrabsKeyboardFocus(false);
    
    tab->addTab("Main", juce::Colours::grey, view, true);
    
    editor->setTab(tab);
 
    editor->setDeviceManager(&deviceManager);
    editor->prepareToPlay( samplesPerBlockExpected, sampleRate);
}

void MainComponent::getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill)
{
    // Your audio-processing code goes here!



    
    // For more details, see the help for AudioProcessor::getNextAudioBlock()

    // Right now we are not producing any data, in which case we need to clear the buffer
    // (to prevent the output of random noise)
    bufferToFill.clearActiveBufferRegion();
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
}

//==============================================================================
void MainComponent::paint (Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));

    // You can add your drawing code here!
}

void MainComponent::resized()
{

    auto r = getLocalBounds().reduced (4);
    
    // make a list of two of our child components that we want to reposition
    Component* comps[] = { propertyView, &resizerBar, tab };
    
    // this will position the 3 components, one above the other, to fit
    // vertically into the rectangle provided.
    stretchableManager.layOutComponents (comps, 3,
                                         r.getX(), r.getY(), r.getWidth(), r.getHeight(),
                                         false, true);
}




