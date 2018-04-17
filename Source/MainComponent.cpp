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
#include "Project.h"
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
    
    toolbar = new Toolbar();
    toolbar->setBounds(0,0,  getWidth(), 50);
    
    addAndMakeVisible(toolbar);
    
    toolbarFactory = new DefaultToolbarItemFactory();
    
    for (int i = 0; i < toolbarFactory->numItems();i++){
        toolbar->addItem(*toolbarFactory, i+1);
        toolbar->getItemComponent(i)->addListener(this);
    }
    
    this->menu = new MenuBarComponent();
    
#if JUCE_MAC
    menu->setModel (nullptr);
    MenuBarModel::setMacMainMenu (this);
#else
    menu->setModel(this);
#endif
    createKeyMap();
    
    for (int i = 0; i < MidiInput::getDevices().size();i++) {
        if (deviceManager.isMidiInputEnabled(MidiInput::getDevices().getReference(i))) {
            deviceManager.addMidiInputCallback(MidiInput::getDevices().getReference(i),this);
        }
    }
    
    editor = new SynthEditor(sampleRate,buffersize);
    // editor->loadFromString(BinaryData::delaytest6);
    
    tab = new MainTabbedComponent();
    tab->setBounds(0,50,getWidth(),getHeight());
    
    view = new Viewport();
    
    addAndMakeVisible(view);
    
    view->setSize(500,200);
    view->setViewedComponent(editor);
    view->setScrollBarsShown(true,true);
    view->setScrollOnDragEnabled(false);
    view->setWantsKeyboardFocus(false);
    view->setMouseClickGrabsKeyboardFocus(false);
    
    tab->addTab("Main", juce::Colours::grey, view, false);
    
    propertyView =  new PropertyView();
    addAndMakeVisible (propertyView);
    addAndMakeVisible (resizerBar);
    addAndMakeVisible (tab);
    
    editor->setTab(tab);

    addKeyListener(this);
    resized();
    
    editor->addChangeListener(propertyView);
    
    // we have to set up our StretchableLayoutManager so it know the limits and preferred sizes of it's contents
    stretchableManager.setItemLayout (0,            // for the properties
                                      -0.1, -0.9,   // must be between 50 pixels and 90% of the available space
                                      -0.2);        // and its preferred size is 30% of the total available space
    
    stretchableManager.setItemLayout (1,            // for the resize bar
                                      5, 5, 5);     // hard limit to 5 pixels
    
    stretchableManager.setItemLayout (2,            // for the imagePreview
                                      -0.1, -0.9,   // size must be between 50 pixels and 90% of the available space
                                      -0.8);
    
    

    
    initialized = true;
    running = true;
}

MainComponent::~MainComponent()
{
    running = false;
    
    editor->removeAllChangeListeners();
    
    for (int i = 0; i < MidiInput::getDevices().size();i++) {
        if (deviceManager.isMidiInputEnabled(MidiInput::getDevices().getReference(i))) {
            deviceManager.removeMidiInputCallback(MidiInput::getDevices().getReference(i),this);
        }
    }
	

#if JUCE_MAC
    MenuBarModel::setMacMainMenu(nullptr);
#endif
    delete editor;
    delete tab;
    delete view;
    delete propertyView;
    delete menu;
    delete toolbar;
    delete toolbarFactory;

    PrefabFactory::getInstance()->destroy();
    Project::getInstance()->destroy();
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();

}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    Logger::writeToLog("prepare to play with sample rate "+String(sampleRate)+" kHz and buffer size of "+String(buffersize)+" bytes.");
    
    this->sampleRate = sampleRate;
    this->buffersize = samplesPerBlockExpected;

    if (editor != nullptr) {
        editor->prepareToPlay(samplesPerBlockExpected, sampleRate);
    }
    
}

void MainComponent::getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill)
{
    if (!running) {
        return;
    }
    // Your audio-processing code goes here!
    int numSamples = bufferToFill.numSamples;
    float** outputChannelData = bufferToFill.buffer->getArrayOfWritePointers();

    
    // for (int i = 0; i < numSamples;i++)
    if (editor->getModule() != nullptr)
        processModule(editor->getModule());
    
    std::vector<AudioOut*> outputChannels = editor->getOutputChannels();
    
    // mute if there are no channels
    if (outputChannels.size() == 0) {
        for (int j = 0;j < numSamples;j++) {
            outputChannelData[0][j] = 0;
            outputChannelData[1][j] = 0;
        }
    }
    
    else {
        
        // process all output pins of the connected module
        // outputChannels.at(0)->getPins().at(0)->process(inputChannelData[0], outputChannelData[0], numSamples);
        for (int j = 0;j < numSamples;j++) {
            
            if (editor->channelIsValid(0)) {
                const float* outL = outputChannels.at(0)->getPins().at(0)->connections.at(0)->getAudioBuffer()->getReadPointer(0);
                outputChannelData[0][j] = outL[j];
            }
            else {
                outputChannelData[0][j] = 0;
            }
            
            if (editor->channelIsValid(1)) {
                const float* outR = outputChannels.at(0)->getPins().at(1)->connections.at(0)->getAudioBuffer()->getReadPointer(0);
                outputChannelData[1][j] = outR[j];
                
            }
            else {
                outputChannelData[1][j] = 0;
            }
        }
    }
    
    // For more details, see the help for AudioProcessor::getNextAudioBlock()

    // Right now we are not producing any data, in which case we need to clear the buffer
    // (to prevent the output of random noise)
    // bufferToFill.clearActiveBufferRegion();
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

    auto r = getLocalBounds().reduced (4).removeFromBottom(getHeight() - 55);
    
    if (toolbar != nullptr)
        toolbar->setSize(getLocalBounds().getWidth() , 50);
    
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
    launchOptions.resizable = false;
    launchOptions.useNativeTitleBar = false;
    launchOptions.useBottomRightCornerResizer = true;
    launchOptions.componentToCentreAround = getParentComponent();
    launchOptions.content.setOwned(selector);
    launchOptions.content->setSize(600, 580);
    launchOptions.dialogBackgroundColour = LookAndFeel::getDefaultLookAndFeel().findColour(ResizableWindow::backgroundColourId);
    launchOptions.runModal();
    
    AudioDeviceManager::AudioDeviceSetup setup;
    deviceManager.getAudioDeviceSetup(setup);
    
    deviceManager.restartLastAudioDevice();
    
    XmlElement* config = deviceManager.createStateXml();
    
    String userHome = File::getSpecialLocation(File::userHomeDirectory).getFullPathName();
    
    File appDir = File(userHome+"/.Synthlab");
    
    if (!appDir.exists()) {
        appDir.createDirectory();
    }
    
    File configFile = File(userHome+"/.Synthlab/config.xml");
    config->writeToFile(configFile,"");
    
    delete config;
    
}

bool MainComponent::keyStateChanged(bool isKeyDown, juce::Component *originatingComponent) {
    for (std::map<int, int>::iterator it = keyCodeMidiNote.begin();it != keyCodeMidiNote.end();it++) {
        if (KeyPress::isKeyCurrentlyDown((*it).first)) {
            sendNoteMessage(editor->getModule(), (*it).second + 12 * currentOctave);
            sendGateMessage(editor->getModule(), 64,(*it).second + 12 * currentOctave, true);
        }
        else {
            if (!isKeyDown)
                sendGateMessage(editor->getModule(), 0,(*it).second + 12 * currentOctave, false);
        }
    }
    return true;
}

bool MainComponent::keyPressed(const juce::KeyPress &key, juce::Component *originatingComponent) {
    if (key.getTextCharacter() == 'y') {
        if (currentOctave > 0) {
            currentOctave--;
        }
    }
    if (key.getTextCharacter() == 'x') {
        if (currentOctave < 8) {
            currentOctave++;
        }
    }
    if (key.getTextCharacter() == 'l') {
        layer = (layer+1) % 4;
        editor->setCurrentLayer(layer);
        editor->repaint();
    }
    return true;
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
  
}

void MainComponent::buttonClicked (Button* b)
{
    ToolbarButton* tb = dynamic_cast<ToolbarButton*>(b);
    if (tb->getItemId() == toolbarFactory->delete_element) {
        editor->removeSelectedItem();
    }
    else if(tb->getItemId() == toolbarFactory->doc_new) {
        editor->newFile();
    }
    else if(tb->getItemId() == toolbarFactory->doc_save) {
        editor->saveFile();
    }
    else if (tb->getItemId() == toolbarFactory->doc_open) {
        editor->openFile();
    }
    else if (tb->getItemId() == toolbarFactory->app_settings) {
        openSettings();
    }
    else if (tb->getItemId() == toolbarFactory->app_undo) {
        Project::getInstance()->getUndoManager()->undo();
    }
    else if (tb->getItemId() == toolbarFactory->app_redo) {
        Project::getInstance()->getUndoManager()->redo();
    }
}

void MainComponent::handleIncomingMidiMessage(juce::MidiInput *source, const juce::MidiMessage &message) {
    
    
    if (message.isNoteOn()) {
        for (int i = 0; i < editor->getModule()->getModules()->size();i++) {
            sendNoteMessage(editor->getModule()->getModules()->at(i), message.getNoteNumber());
            sendGateMessage(editor->getModule()->getModules()->at(i), message.getVelocity(),message.getNoteNumber(),true);
            
        }
    }
    else if (message.isNoteOff()) {
        for (int i = 0; i < editor->getModule()->getModules()->size();i++) {
            sendGateMessage(editor->getModule()->getModules()->at(i), message.getVelocity(),message.getNoteNumber(),false);
        }
    }
    else if(message.isController()) {
        for (int i = 0; i < editor->getModule()->getModules()->size();i++) {
            sendControllerMessage(editor->getModule()->getModules()->at(i), message.getControllerNumber(),message.getControllerValue());
        }
    }
    
    // deviceManager.getDefaultMidiOutput()->sendMessageNow(message);
}

void MainComponent::sendGateMessage(Module *module,int velocity,int note,  bool on) {
    
    
    MidiGate* gate;
    
    if ((gate = dynamic_cast<MidiGate*>(module)) != NULL) {
        if (on) {
            if (velocity > 0)
                gate->gateOn(velocity,note);
        }
        else {
            gate->gateOff(note);
        }
    }
    
    for (int i = 0; i< module->getModules()->size();i++) {
        
        if ((gate = dynamic_cast<MidiGate*>(module->getModules()->at(i)))!= NULL) {
            if (on) {
                gate->gateOn(velocity,note);
            }
            else {
                gate->gateOff(note);
            }
            
            sendGateMessage(module->getModules()->at(i),velocity,note,on);
        }
    }
    
}

void MainComponent::sendNoteMessage(Module *module, int note) {
    
    MidiNote* midiNote;
    
    if ((midiNote = dynamic_cast<MidiNote*>(module)) != NULL) {
        if (note > 0)
            midiNote->note(note);
    }
    
    for (int i = 0; i< module->getModules()->size();i++) {
        
        if ((midiNote = dynamic_cast<MidiNote*>(module->getModules()->at(i)))!= NULL) {
            sendNoteMessage(module->getModules()->at(i), note);
        }
    }
}

void MainComponent::sendControllerMessage(Module *module, int controller, float value) {
    
    Knob* k;
    
    if ((k = dynamic_cast<Knob*>(module)) != NULL) {
        if (k->isLearning()) {
            k->setIsMidicontroller(true);
            k->setController(controller);
            k->setLearning(false);
        }
        if (k->getController() == controller) {
            k->setValue(value);
        }
        
    }
    for (int i = 0; i< module->getModules()->size();i++) {
        
        if ((k = dynamic_cast<Knob*>(module->getModules()->at(i)))!= NULL) {
            sendControllerMessage(module->getModules()->at(i),controller, value);
        }
    }
}

void MainComponent::audioDeviceIOCallback(const float **inputChannelData, int numInputChannels, float **outputChannelData, int numOutputChannels, int numSamples) {
    
    for (int i = 0; i < numSamples;i++)
        processModule(editor->getModule());
    
    std::vector<AudioOut*> outputChannels = editor->getOutputChannels();
    
    // mute if there are no channels
    if (outputChannels.size() == 0) {
        for (int j = 0;j < numSamples;j++) {
            outputChannelData[0][j] = 0;
            outputChannelData[1][j] = 0;
        }
    }
    
    else {
        
        // process all output pins of the connected module
        // outputChannels.at(0)->getPins().at(0)->process(inputChannelData[0], outputChannelData[0], numSamples);
        for (int j = 0;j < numSamples;j++) {
            
            if (editor->channelIsValid(0)) {
                const float* outL = outputChannels.at(0)->getPins().at(0)->connections.at(0)->getAudioBuffer()->getReadPointer(0);
                outputChannelData[0][j] = outL[j];
            }
            else {
                outputChannelData[0][j] = 0;
            }
            
            if (editor->channelIsValid(1)) {
                const float* outR = outputChannels.at(0)->getPins().at(1)->connections.at(0)->getAudioBuffer()->getReadPointer(0);
                outputChannelData[1][j] = outR[j];
                
            }
            else {
                outputChannelData[1][j] = 0;
            }
        }
    }
    
}

void MainComponent::processModule(Module* m) {
    
    if (m != nullptr) {
        
        m->process();
        
        
         for (int i = 0; i< m->getModules()->size();i++) {
             processModule(m->getModules()->at(i));
         }
        
    }
    
}

