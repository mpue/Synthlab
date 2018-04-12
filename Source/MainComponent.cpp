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
    stretchableManager.setItemLayout (0,            // for the properties
                                      -0.1, -0.9,   // must be between 50 pixels and 90% of the available space
                                      -0.2);        // and its preferred size is 30% of the total available space
    
    stretchableManager.setItemLayout (1,            // for the resize bar
                                      5, 5, 5);     // hard limit to 5 pixels
    
    stretchableManager.setItemLayout (2,            // for the imagePreview
                                      -0.1, -0.9,   // size must be between 50 pixels and 90% of the available space
                                      -0.8);
    
    resized();
    
    this->menu = new MenuBarComponent();
    
#if JUCE_MAC
    menu->setModel (nullptr);
    MenuBarModel::setMacMainMenu (this);
#else
    menu->setModel(this);
#endif
    createKeyMap();
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
    
    editor->removeAllChangeListeners();

	delete editor;
    delete tab;
    delete propertyView;
    MenuBarModel::setMacMainMenu(nullptr);
    delete menu;
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
    // view->setScrollOnDragEnabled(true);
    view->setWantsKeyboardFocus(false);
    view->setMouseClickGrabsKeyboardFocus(false);
    
    tab->addTab("Main", juce::Colours::grey, view, true);
    
    editor->setTab(tab);
 
    editor->setDeviceManager(&deviceManager);
    editor->prepareToPlay( samplesPerBlockExpected, sampleRate);
    addKeyListener(this);
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
    
    if (propertyView != nullptr && propertyView->getParentComponent() != NULL)
        propertyView->setSize(r.getWidth()-tab->getWidth(), propertyView->getHeight());
}

PopupMenu MainComponent::getMenuForIndex(int index, const String & menuName) {
    PopupMenu menu;
    
    if (index == 0) {
        menu.addItem(1, "New project", true, false, nullptr);
        menu.addItem(2, "Load project", true, false, nullptr);
        menu.addItem(3, "Save project", true, false, nullptr);
        menu.addItem(4, "Save project as", true, false, nullptr);
        menu.addItem(5, "Settings", true, false, nullptr);
        menu.addItem(999, "Exit", true, false, nullptr);
    }


    
    return menu;
}

void MainComponent::menuItemSelected(int menuItemID, int topLevelMenuIndex) {
    
    if (menuItemID == 1) {
        editor->cleanUp();
    }
    else if (menuItemID == 2) {
        editor->openFile();
    }
    else if (menuItemID == 2) {
        editor->saveFile();
    }
    else if (menuItemID == 4) {
        
    }
    else if (menuItemID == 5) {
        openSettings();
    }
    else if (menuItemID == 999) {
        JUCEApplication::getInstance()->shutdown();
    }
    
}

StringArray MainComponent::getMenuBarNames() {
    const char* const names[] = { "File", nullptr };
    
    return StringArray(names);
    
}

void MainComponent::openSettings() {

    AudioDeviceSelectorComponent* selector = new AudioDeviceSelectorComponent(deviceManager, 2, 16, 2, 16, true, true, true, false);
    DialogWindow::LaunchOptions launchOptions;
    
    launchOptions.dialogTitle = ("Audio Settings");
    launchOptions.escapeKeyTriggersCloseButton = true;
    launchOptions.resizable = true;
    launchOptions.useNativeTitleBar = false;
    launchOptions.useBottomRightCornerResizer = true;
    launchOptions.componentToCentreAround = getParentComponent();
    launchOptions.content.setOwned(selector);
    launchOptions.content->setSize(600, 580);
    launchOptions.runModal();
    
    AudioDeviceManager::AudioDeviceSetup setup;
    deviceManager.getAudioDeviceSetup(setup);
    
    XmlElement* config = deviceManager.createStateXml();
    
    String userHome = File::getSpecialLocation(File::userHomeDirectory).getFullPathName();
    
    File appDir = File(userHome+"/.Synthlab");
    
    if (!appDir.exists()) {
        appDir.createDirectory();
    }
    
    File configFile = File(userHome+"/.Synthlab/config.xml");
    config->writeToFile(configFile,"");
    
}

bool MainComponent::keyStateChanged(bool isKeyDown, juce::Component *originatingComponent) {
    for (std::map<int, int>::iterator it = keyCodeMidiNote.begin();it != keyCodeMidiNote.end();it++) {
        if (KeyPress::isKeyCurrentlyDown((*it).first)) {
            editor->sendNoteMessage(editor->getModule(), (*it).second + 12 * currentOctave);
            editor->sendGateMessage(editor->getModule(), 64, true);
        }
        else {
            if (!isKeyDown)
                editor->sendGateMessage(editor->getModule(), 0, false);
        }
    }
    return true;
}

bool MainComponent::keyPressed(const juce::KeyPress &key, juce::Component *originatingComponent) {
    
}

void MainComponent::createKeyMap() {
    keyCodeMidiNote['a'] = 36 ;
    keyCodeMidiNote['w'] = 36;
    keyCodeMidiNote['s'] = 38;
    keyCodeMidiNote['e'] = 39;
    keyCodeMidiNote['d'] = 40;
    keyCodeMidiNote['f'] = 41;
    keyCodeMidiNote['t'] = 42;
    keyCodeMidiNote['g'] = 43;
    keyCodeMidiNote['z'] = 44;
    keyCodeMidiNote['h'] = 45;
    keyCodeMidiNote['u'] = 46;
    keyCodeMidiNote['j'] = 47;
    keyCodeMidiNote['k'] = 48;
    keyCodeMidiNote['o'] = 49;
    keyCodeMidiNote['l'] = 50;
}

