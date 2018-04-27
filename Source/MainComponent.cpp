/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "Plugins/PluginManager.h"
#include "AudioManager.h"
#include "MainComponent.h"
#include "SynthEditor.h"
#include "PropertyView.h"
#include "MainTabbedComponent.h"
#include "MidiGate.h"
#include "MidiNote.h"
#include "PrefabFactory.h"
#include "Project.h"
#include "Knob.h"
#include "Mixer.h"
#include "PluginModule.h"
#include "ModuleBrowser.h"

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
    
    AudioManager::getInstance()->setDeviceManager(&deviceManager);
    PluginManager::getInstance();
    
    toolbar = new Toolbar();
    toolbar->setBounds(0,0,  getWidth(), 50);
    
    addAndMakeVisible(toolbar);
    
    toolbarFactory = new DefaultToolbarItemFactory();
    
    for (int i = 0; i < toolbarFactory->numItems();i++){
        toolbar->addItem(*toolbarFactory, i+1);
        toolbar->getItemComponent(i)->addListener(this);
        
        ToolbarComboBox* tcb = dynamic_cast<ToolbarComboBox*>(toolbar->getItemComponent(i));
        
        if (tcb != NULL) {
            tcb->getComboBox().onChange = [tcb,this]() {
                int id = tcb->getComboBox().getSelectedId() - 1;
                editor->setCurrentLayer(id);
            };
        }
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
    
    editorView = new EditorComponent(sampleRate, buffersize);
    editor = editorView->getEditor();
    mixer = editorView->getMixer();
    mixerPanel = editorView->getMixerPanel();
    
    propertyView =  new PropertyView();
    
    addAndMakeVisible (propertyView);
    addAndMakeVisible (resizerBar);
    addAndMakeVisible(editorView);
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
    
    stretchableManager.setItemLayout (2,            // for the main editor
                                      -0.1, -0.9,   // size must be between 50 pixels and 90% of the available space
                                      -0.8);
    
    
    pluginMenu = PluginManager::getInstance()->buildPluginMenu();
    
    initialized = true;
    running = true;
    
    cpuLoadLabel = new Label("0%");
    cpuLoadLabel->setText("0%",juce::NotificationType::dontSendNotification);
    toolbar->addAndMakeVisible(cpuLoadLabel);
    
    loadSlider = new Slider();
    
    loadSlider->setRange (0, 100, 1);
    loadSlider->setSliderStyle (Slider::LinearHorizontal);
    loadSlider->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    loadSlider->setColour (Slider::backgroundColourId, Colour (0xff313131));
    loadSlider->setColour (Slider::thumbColourId, Colours::chartreuse);
    loadSlider->setColour (Slider::trackColourId, Colour (0xff434242));
    loadSlider->setColour (Slider::rotarySliderOutlineColourId, Colour (0x66ffffff));
    loadSlider->setColour (Slider::textBoxBackgroundColourId, Colour (0x00ffffff));
    loadSlider->setLookAndFeel(Project::getInstance()->getLookAndFeel());
    
    addAndMakeVisible(loadSlider);
    
    addMouseListener(this, true);
    
    startTimer(20);
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

    delete tab;
    delete view;
    delete propertyView;
    delete menu;
    delete toolbar;
    delete toolbarFactory;
    delete editorView;
    delete pluginMenu;
    delete cpuLoadLabel;
    delete loadSlider;
    
    if(moduleBrowser != nullptr) {
        delete moduleBrowser;
    }

    PrefabFactory::getInstance()->destroy();
    Project::getInstance()->destroy();
    
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

void MainComponent::timerCallback(){
   
    if (mixerPanel != nullptr) {
        for (int i = 0; i < mixer->getChannels().size();i++) {
            mixerPanel->getChannels().at(i)->setMagnitude(0,mixer->getChannels().at(i)->magnitudeLeft);
            mixerPanel->getChannels().at(i)->setMagnitude(1,mixer->getChannels().at(i)->magnitudeRight);
        }
    }
    
    currentMeasure = (currentMeasure +1) % 10;
    
    loads[currentMeasure] = cpuLoad;
    
    if (currentMeasure == 0) {
        for (int i = 0; i < 10;i++) {
            cpuLoad += loads[i];
        }
        cpuLoad /= 10;
        loadSlider->setValue(cpuLoad);
        cpuLoadLabel->setText(String(cpuLoad)+"%", juce::NotificationType::dontSendNotification);
    }
    

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

int MainComponent::getNumActiveChannels(int i) {
    i = i - ((i >> 1) & 0x55555555);
    i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
    return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}


void MainComponent::getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill)
{
    if (!running) {
        return;
    }

    lastTime = Time::getMillisecondCounterHiRes() - currentTime;
    currentTime = Time::getMillisecondCounterHiRes();

    long startTime = currentTime;
    
    int numSamples = bufferToFill.numSamples;
    float** outputChannelData = bufferToFill.buffer->getArrayOfWritePointers();
    const float** inputChannelData = bufferToFill.buffer->getArrayOfReadPointers();
    
    // for (int i = 0; i < numSamples;i++)
    if (editor->getModule() != nullptr)
        processModule(editor->getModule());
    
    std::vector<AudioOut*> outputChannels = editor->getOutputChannels();
    std::vector<AudioIn*> inputChannels = editor->getInputChannels();
    std::vector<AuxOut*> auxChannels = editor->getAuxChannels();

    
    for (int k = 0; k < mixer->getNumInputs();k++) {
        
        Mixer::Channel* input =  mixer->getChannel(Mixer::Channel::Type::IN, k);
        
        if (input != nullptr) {
            inputChannels.at(k)->pins.at(0)->getAudioBuffer()->copyFrom(0, bufferToFill.startSample, *bufferToFill.buffer, k, bufferToFill.startSample, numSamples);
            inputChannels.at(k)->pins.at(1)->getAudioBuffer()->copyFrom(0, bufferToFill.startSample, *bufferToFill.buffer, k+1, bufferToFill.startSample, numSamples);
            input->magnitudeLeft = inputChannels.at(k)->pins.at(0)->getAudioBuffer()->getMagnitude(0, 0, numSamples);
            input->magnitudeRight = inputChannels.at(k)->pins.at(1)->getAudioBuffer()->getMagnitude(1, 0, numSamples);
        }
 
    }
    
    // mute if there are no channels
    if (mixer->getNumOutputs() ==  0) {
        for (int j = 0;j < numSamples;j++) {
            outputChannelData[0][j] = 0;
            outputChannelData[1][j] = 0;
        }
    }
    else {
        
        Mixer::Channel* channel =  mixer->getChannel(Mixer::Channel::Type::OUT, 0);
        float channelVolume = channel->mute ? 0 : channel->volume;
        double pan = channel->pan;
        
        float gainLeft = cos((M_PI*(pan + 1) / 4));
        float gainRight = sin((M_PI*(pan + 1) / 4));

        // process all output pins of the connected module


        for (int j = 0;j < numSamples;j++) {

            float auxLeftOut = 0;
            
            // merge the output of the AUX busses
            
            for (int k = 0; k < auxChannels.size();k++) {
                
                if (editor->auxChannelIsValid(k,0)) {
                    const float* auxL = auxChannels.at(k)->getPins().at(0)->connections.at(0)->getAudioBuffer()->getReadPointer(0);
                    
                    Mixer::Channel* channel =  mixer->getChannel(Mixer::Channel::Type::AUX, k);
                    float auxVolume = channel->mute ? 0 : channel->volume;
                    double auxpan = channel->pan;
                    
                    float auxgainLeft = cos((M_PI*(auxpan + 1) / 4));
                    channel->magnitudeLeft = auxVolume * auxgainLeft * auxChannels.at(k)->getPins().at(0)->connections.at(0)->getAudioBuffer()->getMagnitude(0, 0, numSamples);

                    auxLeftOut += auxL[j] * auxVolume * auxgainLeft;
                    
                }
                
                
            }
            
            if (editor->channelIsValid(0)) {
                // outputChannels.at(0)->getPins().at(0)->connections.at(0)->getAudioBuffer()->applyGain(channelVolume);
                const float* outL = outputChannels.at(0)->getPins().at(0)->connections.at(0)->getAudioBuffer()->getReadPointer(0);
                outputChannelData[0][j] = channelVolume * (outL[j] + auxLeftOut) * gainLeft;
            }
            else {
                outputChannelData[0][j] = auxLeftOut;
            }
            
            float auxRightOut = 0;
            
            for (int k = 0; k < auxChannels.size();k++) {
                
                if (editor->auxChannelIsValid(k,1)) {
                    const float* auxR = auxChannels.at(k)->getPins().at(1)->connections.at(0)->getAudioBuffer()->getReadPointer(0);
                    
                    Mixer::Channel* channel =  mixer->getChannel(Mixer::Channel::Type::AUX, k);
                    float auxVolume = channel->mute ? 0 : channel->volume;
                    double auxpan = channel->pan;
                    
                    float auxgainRight = sin((M_PI*(auxpan + 1) / 4));
                    channel->magnitudeRight = auxVolume * auxgainRight * auxChannels.at(k)->getPins().at(1)->connections.at(0)->getAudioBuffer()->getMagnitude(0, 0, numSamples);
                    auxRightOut += auxR[j] * auxVolume * auxgainRight;
                }
                
                
            }
            
            if (editor->channelIsValid(1)) {
                // outputChannels.at(0)->getPins().at(1)->connections.at(0)->getAudioBuffer()->applyGain(channelVolume);
                const float* outR = outputChannels.at(0)->getPins().at(1)->connections.at(0)->getAudioBuffer()->getReadPointer(0);

               
                outputChannelData[1][j] = channelVolume * (outR[j] + auxRightOut) * gainRight;
                
            }
            else {
                outputChannelData[1][j] = auxRightOut;
            }
        }
        
        if (editor->channelIsValid(0))
            channel->magnitudeLeft = channelVolume * gainLeft * outputChannels.at(0)->getPins().at(0)->connections.at(0)->getAudioBuffer()->getMagnitude(0, 0, numSamples);
        if (editor->channelIsValid(1))
            channel->magnitudeRight  = channelVolume * gainRight * outputChannels.at(0)->getPins().at(1)->connections.at(0)->getAudioBuffer()->getMagnitude(0, 0, numSamples);
    }
    
    long duration = Time::getMillisecondCounterHiRes() - startTime;
    cpuLoad = ((float)duration / (float)lastTime) * 100 ;
    
}

void MainComponent::releaseResources()
{
}

//==============================================================================
void MainComponent::paint (Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{

    auto r = getLocalBounds().reduced (4).removeFromBottom(getHeight() - 55);
    
    if (toolbar != nullptr)
        toolbar->setSize(getLocalBounds().getWidth() , 50);
    
    // make a list of two of our child components that we want to reposition
    Component* comps[] = { propertyView, &resizerBar, editorView };
    
    // this will position the 3 components, one above the other, to fit
    // vertically into the rectangle provided.
    stretchableManager.layOutComponents (comps, 3,
                                         r.getX(), r.getY(), r.getWidth(), r.getHeight(),
                                         false, true);
    
    if (propertyView != nullptr && propertyView->getParentComponent() != NULL)
        propertyView->setSize(r.getWidth()-editorView->getWidth(), propertyView->getHeight());
    if (getParentComponent() != nullptr) {
        cpuLoadLabel->setBounds(getParentComponent()->getWidth() - 50,0,50,20);
        loadSlider->setBounds(getParentComponent()->getWidth() - 150, 10, 100, 5);
    }
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
    else if (index == 1) {
        menu.addItem(11, "Show mixer", true, false, nullptr);
        menu.addItem(12, "Manage plugins", true, false, nullptr);
    }
    else if (index == 2) {
        
        if (pluginMenu != nullptr)
            return *pluginMenu;
    }

    
    return menu;
}

void MainComponent::menuItemSelected(int menuItemID, int topLevelMenuIndex) {
    
    if (menuItemID == 1) {
        editor->setRunning(false);
        editor->cleanUp();
        editor->getMixer()->removeAllChannels();
        editor->newFile();
        editor->setRunning(true);
    }
    else if (menuItemID == 2) {
        editor->setRunning(false);
        editor->openFile();
        editor->setRunning(true);
    }
    else if (menuItemID == 2) {
        editor->saveFile();
    }
    else if (menuItemID == 4) {
        
    }
    else if (menuItemID == 5) {
        openSettings();
    }
    else if (menuItemID == 12) {
        PluginManager::getInstance()->scanPlugins();
        
    }
    else if (menuItemID > 100){
        String pluginName = PluginManager::getInstance()->getAvailablePlugins().at(menuItemID - 100);
        
        if (editor->getSelectionModel()->getSelectedModule() != nullptr) {
            PluginModule *pm = dynamic_cast<PluginModule*>(editor->getSelectionModel()->getSelectedModule());
            
            if (pm != NULL) {
                pm->selectPlugin(pluginName);
            }
        }
        
    }
    else if (menuItemID == 999) {
        JUCEApplication::getInstance()->shutdown();
    }
    
}

StringArray MainComponent::getMenuBarNames() {
    const char* const names[] = { "File", "View","Plugins", nullptr };
    
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
    Project::getInstance()->getOpenWindows().push_back(launchOptions.launchAsync());
    
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
    
    if (config != NULL) {
        config->writeToFile(configFile,"");
        delete config;
    }

}

bool MainComponent::keyStateChanged(bool isKeyDown, juce::Component *originatingComponent) {
    for (std::map<int, int>::iterator it = keyCodeMidiNote.begin();it != keyCodeMidiNote.end();it++) {
        if (KeyPress::isKeyCurrentlyDown((*it).first)) {
            if (!keyStates[(*it).second]) {
                sendNoteMessage(editor->getModule(), (*it).second + 12 * currentOctave);
                sendGateMessage(editor->getModule(), 64,(*it).second + 12 * currentOctave, true);
                keyStates[(*it).second] = true;
            }

        }
        else {
            keyStates[(*it).second] = false;
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
    
    if (tb != NULL) {
        
        if (tb->getItemId() == toolbarFactory->delete_element) {
            editor->removeSelectedItem();
        }
        else if(tb->getItemId() == toolbarFactory->doc_new) {
            editor->setRunning(false);
            editor->cleanUp();
            editor->newFile();
            editor->setRunning(true);
        }
        else if(tb->getItemId() == toolbarFactory->doc_save) {
            editor->saveFile();
        }
        else if (tb->getItemId() == toolbarFactory->doc_open) {
            editor->setRunning(false);
            editor->openFile();
            editor->setRunning(true);
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
        else if (tb->getItemId() == toolbarFactory->mod_sources) {
            
            if (moduleBrowser == nullptr) {
                moduleBrowser = new ModuleBrowser();
                addChildComponent(moduleBrowser);
            }
    
            if (!moduleBrowser->isVisible()) {
                moduleBrowser->setTopLeftPosition(b->getX(), b->getY() + b->getHeight());
                moduleBrowser->setVisible(true);
                moduleBrowser->toFront(true);
            }
            else {
                moduleBrowser->setVisible(false);
            }
    

            
            /*
            PopupMenu pm = PopupMenu();
            
            p->add
            pm.addItem(100, "Sawtooth", true, true, ImageCache::getFromMemory(BinaryData::saw_png, BinaryData::saw_pngSize));
            pm.showAt(b);
             */

        }
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
    
    if (m != nullptr && running) {
        
        m->process();
        
        
         for (int i = 0; i< m->getModules()->size();i++) {
             if (m->getModules()->at(i) != nullptr) {
                 processModule(m->getModules()->at(i));
             }
         }
        
    }
    
}

