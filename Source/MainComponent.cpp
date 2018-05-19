/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/


#include "Plugins/PluginManager.h"
#include "PluginModule.h"

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
#include "Actions/RemoveSelectedAction.h"
#include "ModuleBrowser.h"
#include "PitchBendModule.h"
#include "ModuleUtils.h"
#include "MidiClock.h"

#ifndef M_PI
#define M_PI       3.14159265358979323846 
#endif
using juce::String;
using juce::File;
using juce::MenuBarComponent;
using juce::MidiInput;
using juce::Label;
using juce::Slider;
using juce::ScopedPointer;
using juce::XmlDocument;
using juce::XmlElement;
using juce::Toolbar;
using juce::Colour;
using juce::Colours;
using juce::MouseEvent;
using juce::ChoicePropertyComponent;
using juce::MemoryBlock;
using juce::var;
using juce::Image;
using juce::Logger;
using juce::AudioSourceChannelInfo;
using juce::DragAndDropTarget;
using juce::Time;
using juce::Graphics;
using juce::PopupMenu;
using juce::ResizableWindow;
using juce::JUCEApplication;
using juce::StringArray;
using juce::AudioDeviceSelectorComponent;
using juce::DialogWindow;
using juce::LookAndFeel;
using juce::AudioDeviceManager;
using juce::ModalComponentManager;
using juce::ModalCallbackFunction;
using juce::KeyPress;
using juce::Button;
using juce::ToolbarButton;

//==============================================================================
MainComponent::MainComponent() : resizerBar (&stretchableManager, 1, true)
{
    
    // Make sure you set the size of the component after
    // you add any child components.
    setSize (1280, 800);

    // specify the number of input and output channels that we want to open
    setAudioChannels (2, 2);

    createConfig();
    
    AudioManager::getInstance()->setDeviceManager(&deviceManager);
    Project::getInstance()->setCommandManager(this);

    if (Project::getInstance()->getAppMode() == Project::AppMode::STUDIO) {
        
#if defined(JUCE_PLUGINHOST_AU) || defined(JUCE_PLUGINHOST_VST)
        PluginManager::getInstance();
#endif
        
        createToolbar();
        createCPUMeter();
        createMenu();
        createKeyMap();
        createStudioLayout();
        
        // a global sampler object which allows us to play audio at any place like for preview for example
        defaultSampler = new Sampler(sampleRate, buffersize);
        Project::getInstance()->setDefaultSampler(defaultSampler);
    }
    else if(Project::getInstance()->getAppMode() == Project::AppMode::PLAYER) {
        createPlayerLayout();
    }
        
    Project::getInstance()->setMain(this);

    startTimer(20);    
    enableAllMidiInputs();
    
    addMouseListener(this, true);
    editor->addKeyListener(getKeyMappings());
    addKeyListener(getKeyMappings());
    addKeyListener(this);
    editor->setMouseClickGrabsKeyboardFocus(true);
    editor->setWantsKeyboardFocus(true);
    
    resized();
    repaint();
    initialized = true;
    running = true;
    
    if (Project::getInstance()->getAppMode() == Project::AppMode::PLAYER) {
        editor->loadFromString(String(BinaryData::_3oscV2_slb ));
        editor->setCurrentLayer(Module::Layer::GUI);
        editor->setLocked(true);
    }
    
}

MainComponent::~MainComponent()
{
    running = false;
    
    if(Project::getInstance()->getAppMode() == Project::AppMode::STUDIO) {
        loadSlider->setLookAndFeel(nullptr);
        
#if JUCE_MAC
        MenuBarModel::setMacMainMenu(nullptr);
#endif
    }

    editor->removeAllChangeListeners();
    
    disableAllMidiInputs();

    if(moduleBrowser != nullptr) {
        if (moduleBrowser->isVisible()) {
            moduleBrowser->setVisible(false);
        }
        delete moduleBrowser;
    }
    
    if(Project::getInstance()->getAppMode() == Project::AppMode::STUDIO) {
        delete tab;
        delete view;
        delete propertyView;
        delete lockButton;
        delete menu;
        delete toolbar;
        delete toolbarFactory;
        delete editorView;
#if defined(JUCE_PLUGINHOST_AU)
        delete pluginMenu;
#endif
        delete cpuLoadLabel;
        delete loadSlider;

    }
    else if(Project::getInstance()->getAppMode() == Project::AppMode::PLAYER) {
        delete editor;
        delete tab;
        delete propertyView;
        delete toolbarFactory;
        delete editorView;
        delete mixer;
        delete mixerPanel;
        delete lockButton;
    }
    
    if (defaultSampler != nullptr) {
        delete defaultSampler;
    }


    PrefabFactory::getInstance()->destroy();
    Project::getInstance()->destroy();
    
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

void MainComponent::createMenu() {
    this->menu = new MenuBarComponent();
    
#if JUCE_MAC
    menu->setModel (nullptr);
    MenuBarModel::setMacMainMenu (this);
#else
    menu->setModel(this);
	menu->setBounds(0, 0, 1000, 25);
    addAndMakeVisible(menu);
#endif
}

void MainComponent::createConfig() {
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
}

void MainComponent::createToolbar() {
    toolbar = new Toolbar();

#if JUCE_MAC
	
#else
    toolbar->setBounds(0,25,  getWidth(), 50);
#endif
    
    addAndMakeVisible(toolbar);
    
    toolbarFactory = new DefaultToolbarItemFactory();
    
    for (int i = 0; i < toolbarFactory->numItems();i++){
        toolbar->addItem(*toolbarFactory, i+1);
        toolbar->getItemComponent(i)->addListener(this);
        
        if (layerCombobox == NULL) {
            layerCombobox = dynamic_cast<ToolbarComboBox*>(toolbar->getItemComponent(i));
        }
        if (layerCombobox != NULL) {
            
            layerCombobox->getComboBox().addListener(this);
        }
    }
    
    lockButton = new ImageButton("Lock");
    
    Image normalImage = juce::ImageCache::getFromMemory(BinaryData::unlock_png, BinaryData::unlock_pngSize);
    Image downImage = juce::ImageCache::getFromMemory(BinaryData::lock_png, BinaryData::lock_pngSize);
    
    lockButton->setImages(false, true, true, normalImage, 1.0f, juce::Colours::white, normalImage, 1.0, juce::Colours::white, downImage, 1.0f, juce::Colours::white);
    lockButton->setClickingTogglesState(true);
    lockButton->setSize(24, 24);
    lockButton->setTopLeftPosition(700,10);
    toolbar->addAndMakeVisible(lockButton);
    
    lockButton->addListener(this);
    
}

void MainComponent::createCPUMeter() {
    cpuLoadLabel = new Label("0%");
    cpuLoadLabel->setText("0%",juce::NotificationType::dontSendNotification);
    toolbar->addAndMakeVisible(cpuLoadLabel);
    
    loadSlider = new Slider();
    
    loadSlider->setRange (0, 100, 1);
    loadSlider->setSliderStyle (Slider::LinearBar);
    loadSlider->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    loadSlider->setColour (Slider::backgroundColourId, Colour (0xff313131));
    loadSlider->setColour (Slider::thumbColourId, Colours::chartreuse);
    loadSlider->setColour (Slider::trackColourId, Colour (0xff434242));
    loadSlider->setColour (Slider::rotarySliderOutlineColourId, Colour (0x66ffffff));
    loadSlider->setColour (Slider::textBoxBackgroundColourId, Colour (0x00ffffff));
    loadSlider->setLookAndFeel(Project::getInstance()->getLookAndFeel());
    
    addAndMakeVisible(loadSlider);
}

void MainComponent::createStudioLayout() {
    editorView = new EditorComponent(sampleRate, buffersize);
    editor = editorView->getEditor();
    mixer = editorView->getMixer();
    mixerPanel = editorView->getMixerPanel();
    
    propertyView =  new PropertyView();
#if JUCE_MAC

#else
	propertyView->setTopLeftPosition(0, 75);
#endif
    editor->addEditorListener(propertyView);
    propertyView->getBrowser()->addChangeListener(editor);
    
    Project::getInstance()->setPropertyView(propertyView);
    
    addAndMakeVisible (propertyView);
    addAndMakeVisible (resizerBar);
    addAndMakeVisible(editorView);
    editor->setTab(editorView->getEditorTab());
    editorView->getEditorTab()->addChangeListener(this);
    editor->addEditorListener(this);
    addMouseListener(propertyView->getBrowser(), false);
    
    registerAllCommandsForTarget(editor);
    setFirstCommandTarget(editor);
    
    // we have to set up our StretchableLayoutManager so it know the limits and preferred sizes of it's contents
    stretchableManager.setItemLayout (0,            // for the properties
                                      -0.1, -0.9,   // must be between 50 pixels and 90% of the available space
                                      -0.2);        // and its preferred size is 30% of the total available space
    
    stretchableManager.setItemLayout (1,            // for the resize bar
                                      5, 5, 5);     // hard limit to 5 pixels
    
    stretchableManager.setItemLayout (2,            // for the main editor
                                      -0.1, -0.9,   // size must be between 50 pixels and 90% of the available space
                                      -0.8);
    
#if defined(JUCE_PLUGINHOST_AU) || defined(JUCE_PLUGINHOST_VST)
    pluginMenu = PluginManager::getInstance()->buildPluginMenu();
#endif
    
}

void MainComponent::createPlayerLayout() {
    editor = new SynthEditor(sampleRate, buffersize);
    mixerPanel = new MixerPanel();
    mixer = new Mixer();
    mixerPanel->setMixer(mixer);
    editor->setMixer(mixerPanel);
    addAndMakeVisible(editor);
    addChildComponent(mixerPanel);
}

void MainComponent::timerCallback(){
   
    if (mixerPanel != nullptr) {
        for (int i = 0; i < mixer->getChannels().size();i++) {
            mixerPanel->getChannels().at(i)->setMagnitude(0,mixer->getChannels().at(i)->magnitudeLeft);
            mixerPanel->getChannels().at(i)->setMagnitude(1,mixer->getChannels().at(i)->magnitudeRight);
        }
    }
    
    if (Project::getInstance()->getAppMode() == Project::AppMode::STUDIO) {
    
        currentMeasure = (currentMeasure +1) % 10;
        
        loads[currentMeasure] = cpuLoad;
        
        if (currentMeasure == 0) {
            for (int i = 0; i < 10;i++) {
                cpuLoad += loads[i];
            }
            cpuLoad /= 10;
            
            if (cpuLoad < 0) {
                cpuLoad = 0;
            }
            if (cpuLoad == NAN) {
                cpuLoad = 0;
            }
            
            loadSlider->setValue(cpuLoad);
            cpuLoadLabel->setText(String(cpuLoad)+"%", juce::NotificationType::dontSendNotification);
        }
    }

}

void MainComponent::mouseDrag (const MouseEvent& event) {
    
    if(Project::getInstance()->getAppMode() == Project::AppMode::PLAYER) {
        return;
    }
    
    var description;
    /*
    TabBarButton* t = dynamic_cast<TabBarButton*>(event.originalComponent);
    
    if (t != nullptr) {
        description.append("tab");
        Logger::writeToLog("Drag from tab "+t->getParentComponent()->getName());
        startDragging(description,t->getParentComponent());
    }
    else {
     */
        description.append("property");
        startDragging(description,propertyView->getBrowser());
    //}
     
}

void MainComponent::dragOperationStarted (const DragAndDropTarget::SourceDetails& details)  {
    
    if(Project::getInstance()->getAppMode() == Project::AppMode::PLAYER) {
        return;
    }
    
    /*
    TabbedButtonBar* tbb = dynamic_cast<TabbedButtonBar*>(details.sourceComponent.get());
    
    if(tbb != nullptr) {

        
        int index = tbb->getCurrentTabIndex();
        
        MainTabbedComponent* tab = dynamic_cast<MainTabbedComponent*>(tbb->getParentComponent());
        
        if (tab != nullptr) {
             setDragImageForIndex(0,tab->getComponentAt(index)->createComponentSnapshot(tab->getComponentAt(index)->getLocalBounds()));
        }
    }
    
    
    else {
     */
        setDragImageForIndex(0, Image());
    // }
    
    
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
    lastTime = Time::getMillisecondCounterHiRes() - currentTime;
    currentTime = Time::getMillisecondCounterHiRes();
    
    long startTime = currentTime;
    
    int numSamples = bufferToFill.numSamples;
    float** outputChannelData = bufferToFill.buffer->getArrayOfWritePointers();
    
    if (outputChannelData == NULL) {
        return;
    }
    
    // const float** inputChannelData = bufferToFill.buffer->getArrayOfReadPointers();
    
    if (defaultSampler != nullptr && defaultSampler->isPlaying()) {
        for (int j = 0;j < numSamples;j++) {
            outputChannelData[0][j] += defaultSampler->getOutput(0);
            outputChannelData[1][j] += defaultSampler->getOutput(1);
            defaultSampler->nextSample();
        }
    }
    
    if (!running) {
        return;
    }
    
    if (editor->getModule() != nullptr)
        processModule(editor->getModule());
    
    std::vector<AudioOut*> outputChannels = editor->getMixer()->getOutputChannels();
    std::vector<AudioIn*> inputChannels = editor->getMixer()->getInputChannels();
    std::vector<AuxOut*> auxChannels = editor->getMixer()->getAuxChannels();
    
    for (int k = 0; k < mixer->getNumInputs();k++) {
        
        Mixer::Channel* input =  mixer->getChannel(Mixer::Channel::Type::IN, k);
        
        if (input != nullptr) {
            
            double pan = input->pan;
            
            float gainLeft = cos((M_PI*(pan + 1) / 4));
            float gainRight = sin((M_PI*(pan + 1) / 4));
            
            inputChannels.at(k)->pins.at(0)->getAudioBuffer()->copyFrom(0, bufferToFill.startSample, *bufferToFill.buffer, k, bufferToFill.startSample, numSamples);
            inputChannels.at(k)->pins.at(1)->getAudioBuffer()->copyFrom(0, bufferToFill.startSample, *bufferToFill.buffer, k+1, bufferToFill.startSample, numSamples);
            inputChannels.at(k)->pins.at(0)->getAudioBuffer()->applyGain(0, bufferToFill.startSample, numSamples, input->volume * gainLeft);
            inputChannels.at(k)->pins.at(1)->getAudioBuffer()->applyGain(0, bufferToFill.startSample, numSamples, input->volume * gainRight);
            input->magnitudeLeft = inputChannels.at(k)->pins.at(0)->getAudioBuffer()->getMagnitude(0, bufferToFill.startSample, numSamples);
            input->magnitudeRight = inputChannels.at(k)->pins.at(1)->getAudioBuffer()->getMagnitude(0, bufferToFill.startSample, numSamples);
        }
 
    }
    
    // mute if there are no channels
    if (mixer->getNumOutputs() ==  0 ) {
        
       if (defaultSampler != nullptr && defaultSampler->isPlaying()) {
       }
       else {
           for (int j = 0;j < numSamples;j++) {
               
               if (outputChannelData[0] != NULL) {
                   outputChannelData[0][j] = 0;
               }
               if (outputChannelData[1] != NULL) {
                   outputChannelData[1][j] = 0;
               }

           }
       }
    }
    else {
        
        Mixer::Channel* outputChannel =  mixer->getChannel(Mixer::Channel::Type::OUT, 0);
        float channelVolume = outputChannel->mute ? 0 : outputChannel->volume;
        double pan = outputChannel->pan;
        
        float gainLeft = cos((M_PI*(pan + 1) / 4));
        float gainRight = sin((M_PI*(pan + 1) / 4));

        // process all output pins of the connected module

        for (int j = 0;j < numSamples;j++) {

            float auxLeftOut = 0;
            
            // merge the output of the AUX busses
            
            for (int k = 0; k < auxChannels.size();k++) {
                
                if (editor->getMixer()->auxChannelIsValid(k,0)) {
                    const float* auxL = auxChannels.at(k)->getPins().at(0)->getConnections().at(0)->getAudioBuffer()->getReadPointer(0);
                    
                    Mixer::Channel* channel =  mixer->getChannel(Mixer::Channel::Type::AUX, k);
                    if (channel != NULL) {
                        float auxVolume = channel->mute ? 0 : channel->volume;
                        double auxpan = channel->pan;
                        
                        float auxgainLeft = cos((M_PI*(auxpan + 1) / 4));
                        channel->magnitudeLeft = auxVolume * auxgainLeft * auxChannels.at(k)->getPins().at(0)->getConnections().at(0)->getAudioBuffer()->getMagnitude(0, 0, numSamples);
                        
                        auxLeftOut += auxL[j] * auxVolume * auxgainLeft;
                    }
                    
                }
                
            }
            
            if (editor->getMixer()->channelIsValid(0)) {
                // outputChannels.at(0)->getPins().at(0)->connections.at(0)->getAudioBuffer()->applyGain(channelVolume);
                const float* outL = outputChannels.at(0)->getPins().at(0)->getConnections().at(0)->getAudioBuffer()->getReadPointer(0);
                outputChannelData[0][j] += channelVolume * (outL[j] + auxLeftOut) * gainLeft;
            }
            else {
                outputChannelData[0][j] = auxLeftOut;
            }
            
            float auxRightOut = 0;
            
            for (int k = 0; k < auxChannels.size();k++) {
                
                if (editor->getMixer()->auxChannelIsValid(k,1)) {
                    const float* auxR = auxChannels.at(k)->getPins().at(1)->getConnections().at(0)->getAudioBuffer()->getReadPointer(0);
                    
                    Mixer::Channel* channel =  mixer->getChannel(Mixer::Channel::Type::AUX, k);
                    
                    if (channel != nullptr) {
                        
                        float auxVolume = channel->mute ? 0 : channel->volume;
                        double auxpan = channel->pan;
                        
                        float auxgainRight = sin((M_PI*(auxpan + 1) / 4));
                        channel->magnitudeRight = auxVolume * auxgainRight * auxChannels.at(k)->getPins().at(1)->getConnections().at(0)->getAudioBuffer()->getMagnitude(0, 0, numSamples);
                        auxRightOut += auxR[j] * auxVolume * auxgainRight;
                    }
                }
                
            }
            
            if (editor->getMixer()->channelIsValid(1)) {
                // outputChannels.at(0)->getPins().at(1)->connections.at(0)->getAudioBuffer()->applyGain(channelVolume);
                const float* outR = outputChannels.at(0)->getPins().at(1)->getConnections().at(0)->getAudioBuffer()->getReadPointer(0);
                if (outputChannelData[1] != NULL)
                    outputChannelData[1][j] += channelVolume * (outR[j] + auxRightOut) * gainRight;
            }
            else {
                if (outputChannelData[1] != NULL)
                    outputChannelData[1][j] = auxRightOut;
            }
        }

        if (editor->getMixer()->channelIsValid(0))
            outputChannel->magnitudeLeft = channelVolume * gainLeft * outputChannels.at(0)->getPins().at(0)->getConnections().at(0)->getAudioBuffer()->getMagnitude(0, 0, numSamples);
        if (editor->getMixer()->channelIsValid(1))
            outputChannel->magnitudeRight  = channelVolume * gainRight * outputChannels.at(0)->getPins().at(1)->getConnections().at(0)->getAudioBuffer()->getMagnitude(0, 0, numSamples);
    }
    
    long duration = Time::getMillisecondCounterHiRes() - startTime;
    cpuLoad = ((float)duration / (float)lastTime) * 100 ;
    
    currentSample = (currentSample + 1) % numSamples;
}

void MainComponent::releaseResources()
{
}

//==============================================================================
void MainComponent::paint (Graphics& g)
{
    g.fillAll (Colour (0xff222222));
}

void MainComponent::resized()
{

    if (Project::getInstance()->getAppMode() == Project::AppMode::STUDIO) {
        auto r = getLocalBounds().reduced (4).removeFromBottom(getHeight() - 55);
        
        if (toolbar != nullptr)
            toolbar->setSize(getLocalBounds().getWidth() , 50);
        
        // make a list of two of our child components that we want to reposition
        Component* comps[] = { propertyView, &resizerBar, editorView };
        
        // this will position the 3 components, one above the other, to fit
        // vertically into the rectangle provided.
        stretchableManager.layOutComponents (comps, 3,
                                             r.getX(), r.getY()+25, r.getWidth(), r.getHeight(),
                                             false, true);
        
        if (propertyView != nullptr && propertyView->getParentComponent() != NULL)
            propertyView->setSize(r.getWidth()-editorView->getWidth(), propertyView->getHeight());
        if (getParentComponent() != nullptr) {
            if (cpuLoadLabel != NULL) {
                cpuLoadLabel->setBounds(getParentComponent()->getWidth() - 50,0,50,20);
                loadSlider->setBounds(getParentComponent()->getWidth() - 150, 10, 100, 10);
            }
            if (editorView != NULL) {
                editorView->getEditor()->resized();
            }
            if (menu != NULL) {
                menu->setBounds(0, 0, getParentWidth(), 25);
            }
            
        }
    }
    else {
        if (getParentComponent() != nullptr && editorView != nullptr) {
            editorView->setSize(getParentWidth(), getParentHeight());
            editorView->getEditor()->resized();
        }
    }
    

}

PopupMenu MainComponent::getMenuForIndex(int index, const String & menuName) {
    PopupMenu menu;
    
    if (index == 0) {
        menu.addCommandItem(this, SynthEditor::CommandIds::NEW);
        menu.addCommandItem(this, SynthEditor::CommandIds::LOAD);
        menu.addCommandItem(this, SynthEditor::CommandIds::SAVE);
        menu.addItem(5, "Settings", true, false, nullptr);
        
        PopupMenu samplesMenu = PopupMenu();
        
        for (int i = 0; i < BinaryData::namedResourceListSize;i++) {
            if (juce::String(BinaryData::namedResourceList[i]).contains("slb")) {
                samplesMenu.addItem(1000+i,String(BinaryData::namedResourceList[i]));
                
                int size;
                const char* data = BinaryData::getNamedResource(BinaryData::namedResourceList[i], size);
                String xmlData = String(data);
                sampleData.add(xmlData);
            }
        }
        
        menu.addSubMenu("Samples",samplesMenu);
        
        
        menu.addItem(999, "Exit", true, false, nullptr);
    }
    else if (index == 1) {
#if defined(JUCE_PLUGINHOST_AU) || defined(JUCE_PLUGINHOST_VST)
        menu.addItem(200, "Manage plugins", true, false, nullptr);
#endif
    }
    else if (index == 2) {
        
        if (pluginMenu != nullptr)
            return *pluginMenu;
    }
    else if (index == 3) {
        menu.addCommandItem(this, SynthEditor::CommandIds::DUPLICATE);
        menu.addCommandItem(this, SynthEditor::CommandIds::DELETE_SELECTED);
        menu.addCommandItem(this, SynthEditor::CommandIds::RESET_GUI_POS);
        
        PopupMenu alignMenu = PopupMenu();
        
        alignMenu.addCommandItem(this, SynthEditor::CommandIds::ALIGN_X);
        alignMenu.addCommandItem(this, SynthEditor::CommandIds::ALIGN_Y);
        
        menu.addSubMenu("Align",alignMenu);
    }
    
    return menu;
}

void MainComponent::menuItemSelected(int menuItemID, int topLevelMenuIndex) {

    if (menuItemID == 5) {
        openSettings();
    }
#if defined(JUCE_PLUGINHOST_AU) || defined(JUCE_PLUGINHOST_VST)
    else if (menuItemID == 200) {
        PluginManager::getInstance()->scanPlugins();
    }
#endif
    else if (menuItemID == 31) {
        editor->duplicateSelected();
        
    }
#if defined(JUCE_PLUGINHOST_AU) || defined(JUCE_PLUGINHOST_VST)
    else if (menuItemID > 100 && menuItemID < 999) {
        String pluginName = PluginManager::getInstance()->getAvailablePlugins().at(menuItemID - 100);
        
        if (editor->getSelectionModel().getSelectedModule() != nullptr) {
            PluginModule *pm = dynamic_cast<PluginModule*>(editor->getSelectionModel().getSelectedModule());
            
            if (pm != NULL) {
                pm->selectPlugin(pluginName);
            }
        }
        
    }
#endif
    else if (menuItemID == 999) {
        JUCEApplication::getInstance()->shutdown();
    }
    else if (menuItemID >= 1000) {
        String xmlData = sampleData.getReference(menuItemID-1000);
        editor->setRunning(false);
        editor->cleanUp();
        editor->newFile();
        editor->loadFromString(xmlData);
        editor->setRunning(true);
    }
}

StringArray MainComponent::getMenuBarNames() {
    
#if defined(JUCE_PLUGINHOST_AU) || defined(JUCE_PLUGINHOST_VST)
    const char* const names[] = { "File", "View","Plugins", "Edit", nullptr };
#else
    const char* const names[] = { "File", "View", "Edit", nullptr };
#endif
    
    return StringArray(names);
    
}

void MainComponent::openSettings() {

    AudioDeviceSelectorComponent* selector = new AudioDeviceSelectorComponent(deviceManager, 2, 16, 2, 16, true, true, true, false);
    DialogWindow::LaunchOptions launchOptions;
    
    selector->setLookAndFeel(Project::getInstance()->getLookAndFeel());
    launchOptions.dialogTitle = ("Audio Settings");
    launchOptions.escapeKeyTriggersCloseButton = true;
    launchOptions.resizable = false;
    launchOptions.useNativeTitleBar = false;
    launchOptions.useBottomRightCornerResizer = true;
    launchOptions.componentToCentreAround = getParentComponent();
    launchOptions.content.setOwned(selector);
    launchOptions.content->setSize(600, 580);
    launchOptions.dialogBackgroundColour = Colour(0xff222222);
    
    DialogWindow* window = launchOptions.launchAsync();
    
    std::function<void(int)> lambda =
    [=](int result) {
        AudioDeviceManager::AudioDeviceSetup setup;
        
        deviceManager.getAudioDeviceSetup(setup);
        deviceManager.restartLastAudioDevice();
        AudioManager::getInstance()->setupChannels();
        refreshMidiInputs();        
        
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
    };
    
    
    ModalComponentManager::Callback* callback = ModalCallbackFunction::create(lambda);
    ModalComponentManager::getInstance()->attachCallback(window, callback);
    
    Project::getInstance()->getOpenWindows().push_back(window);
 
}

bool MainComponent::keyStateChanged(bool isKeyDown, juce::Component *originatingComponent) {
    
    for (std::map<int, int>::iterator it = keyCodeMidiNote.begin();it != keyCodeMidiNote.end();it++) {
        if (KeyPress::isKeyCurrentlyDown((*it).first)) { 
            if (!keyStates[(*it).second]) {
                sendNoteMessage(editor->getModule(),1, (*it).second + 12 * currentOctave);
                sendGateMessage(editor->getModule(),1, 64,(*it).second + 12 * currentOctave, true);
                keyStates[(*it).second] = true;
            }
        }
        else {
            keyStates[(*it).second] = false;
            if (!isKeyDown) {
                sendGateMessage(editor->getModule(), 1,0,(*it).second + 12 * currentOctave, false);
            }
        }
    }
    return true;
}

void MainComponent::modifierKeysChanged(const juce::ModifierKeys &modifiers) {
    isOptionPressed = modifiers.isAltDown();
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
    if (key.getTextCharacter() == 'd' && isOptionPressed) {
        editor->duplicateSelected();
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
            RemoveSelectedAction* rma = new RemoveSelectedAction(editor);
            Project::getInstance()->getUndoManager()->beginNewTransaction();
            Project::getInstance()->getUndoManager()->perform(rma);
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

        }
    }

    if (b == lockButton) {
        editor->setLocked(lockButton->getToggleState());
    }
    
}

void MainComponent::handleIncomingMidiMessage(juce::MidiInput *source, const juce::MidiMessage &message) {
    
    if (message.isNoteOn() && message.getNoteNumber() > 7) {
        for (int i = 0; i < editor->getModule()->getModules()->size();i++) {
            sendNoteMessage(editor->getModule()->getModules()->at(i), message.getChannel(),message.getNoteNumber());
            sendGateMessage(editor->getModule()->getModules()->at(i), message.getChannel(),message.getVelocity(),message.getNoteNumber(),true);
        }
    }
    else if (message.isNoteOff() && message.getNoteNumber() > 7) {
        for (int i = 0; i < editor->getModule()->getModules()->size();i++) {
            sendGateMessage(editor->getModule()->getModules()->at(i), message.getChannel(),message.getVelocity(),message.getNoteNumber(),false);
        }
    }
    else if(message.isController()) {
        for (int i = 0; i < editor->getModule()->getModules()->size();i++) {
            sendControllerMessage(editor->getModule()->getModules()->at(i), message.getChannel(),message.getControllerNumber(),message.getControllerValue());
        }
    }
    else if(message.isPitchWheel()) {
        for (int i = 0; i < editor->getModule()->getModules()->size();i++) {
            sendPitchBendMessage(editor->getModule()->getModules()->at(i), message.getChannel(),message.getPitchWheelValue());
        }
    }
    else if (message.isMidiClock()) {
        for (int i = 0; i < editor->getModule()->getModules()->size();i++) {
            sendClockMessage(editor->getModule()->getModules()->at(i), message.getChannel());
        }
    }
    else if (message.isMidiStart()) {
        for (int i = 0; i < editor->getModule()->getModules()->size();i++) {
            sendMidiStartMessage(editor->getModule()->getModules()->at(i), message.getChannel());
        }
    }
    else if (message.isMidiMachineControlMessage()) {
        for (int i = 0; i < editor->getModule()->getModules()->size();i++) {
            sendMidiStopMessage(editor->getModule()->getModules()->at(i), message.getChannel());
        }
    }
    else {
        Logger::writeToLog(message.getDescription());
    }
    
    // deviceManager.getDefaultMidiOutput()->sendMessageNow(message);
}

void MainComponent::sendGateMessage(Module *module,int channel, int velocity,int note,  bool on) {
    
    MidiGate* gate;
    
    if ((gate = dynamic_cast<MidiGate*>(module)) != NULL) {
        if (gate->getChannel() == channel) {
            if (on) {
                if (velocity > 0)
                    gate->gateOn(velocity,note);
            }
            else {
                gate->gateOff(note);
            }
        }

    }
    
    for (int i = 0; i< module->getModules()->size();i++) {

        if ((gate = dynamic_cast<MidiGate*>(module->getModules()->at(i)))!= NULL) {
            if (gate->getChannel() == channel) {
                if (on) {
                    gate->gateOn(velocity,note);
                }
                else {
                    gate->gateOff(note);
                }
                
                sendGateMessage(module->getModules()->at(i), channel,velocity,note,on);
            }
        }
    }

}

void MainComponent::sendNoteMessage(Module *module, int channel, int note) {
    
    MidiNote* midiNote;
    
    if ((midiNote = dynamic_cast<MidiNote*>(module)) != NULL) {
        if (note > 0)
            midiNote->note(note);
    }
    
    for (int i = 0; i< module->getModules()->size();i++) {
        
        if ((midiNote = dynamic_cast<MidiNote*>(module->getModules()->at(i)))!= NULL) {
            sendNoteMessage(module->getModules()->at(i),channel, note);
        }
    }
}

void MainComponent::sendClockMessage(Module *module, int channel) {
    
    MidiClock* midiClock;
    
    if ((midiClock = dynamic_cast<MidiClock*>(module)) != NULL) {
        midiClock->clock(MidiMessage::midiClock());
    }
    
    for (int i = 0; i< module->getModules()->size();i++) {
        
        if ((midiClock = dynamic_cast<MidiClock*>(module->getModules()->at(i)))!= NULL) {
            sendClockMessage(module->getModules()->at(i),channel);
        }
    }
}

void MainComponent::sendMidiStartMessage(Module *module, int channel) {
    
    MidiClock* midiClock;
    
    if ((midiClock = dynamic_cast<MidiClock*>(module)) != NULL) {
        midiClock->midiStart();
    }
    
    for (int i = 0; i< module->getModules()->size();i++) {
        
        if ((midiClock = dynamic_cast<MidiClock*>(module->getModules()->at(i)))!= NULL) {
            sendMidiStartMessage(module->getModules()->at(i),channel);
        }
    }
}

void MainComponent::sendMidiStopMessage(Module *module, int channel) {
    
    MidiClock* midiClock;
    
    if ((midiClock = dynamic_cast<MidiClock*>(module)) != NULL) {
        midiClock->midiStop();
    }
    
    for (int i = 0; i< module->getModules()->size();i++) {
        
        if ((midiClock = dynamic_cast<MidiClock*>(module->getModules()->at(i)))!= NULL) {
            sendMidiStopMessage(module->getModules()->at(i),channel);
        }
    }
}

void MainComponent::sendPitchBendMessage(Module *module, int channel, int pitch) {
    
    PitchBendModule* pbm;
    
    if ((pbm = dynamic_cast<PitchBendModule*>(module)) != NULL) {
        pbm->setPitch(pitch);
    }
    
    for (int i = 0; i< module->getModules()->size();i++) {
        if ((pbm = dynamic_cast<PitchBendModule*>(module->getModules()->at(i)))!= NULL) {
            sendPitchBendMessage(module->getModules()->at(i),channel, pitch);
        }
    }
}

void MainComponent::sendControllerMessage(Module *module, int channel, int controller, float value) {
    
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
            sendControllerMessage(module->getModules()->at(i), channel, controller, value);
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

void MainComponent::refreshMidiInputs() {
    for (int i = 0; i < MidiInput::getDevices().size();i++) {
        if (deviceManager.isMidiInputEnabled(MidiInput::getDevices().getReference(i))) {
            deviceManager.removeMidiInputCallback(MidiInput::getDevices().getReference(i),this);
            deviceManager.addMidiInputCallback(MidiInput::getDevices().getReference(i),this);
        }
    }
}

void MainComponent::enableAllMidiInputs() {
    for (int i = 0; i < MidiInput::getDevices().size();i++) {
        if (deviceManager.isMidiInputEnabled(MidiInput::getDevices().getReference(i))) {
            deviceManager.addMidiInputCallback(MidiInput::getDevices().getReference(i),this);
        }
    }
}

void MainComponent::disableAllMidiInputs() {
    for (int i = 0; i < MidiInput::getDevices().size();i++) {
        if (deviceManager.isMidiInputEnabled(MidiInput::getDevices().getReference(i))) {
            deviceManager.removeMidiInputCallback(MidiInput::getDevices().getReference(i),this);
        }
    }
}

void MainComponent::changeListenerCallback(juce::ChangeBroadcaster *source) {
    
    SynthEditor* editor = dynamic_cast<SynthEditor*>(source);
    if (editor != nullptr) {
        
        
    }
}

void MainComponent::comboBoxChanged(juce::ComboBox *comboBoxThatHasChanged) {
    int id = comboBoxThatHasChanged->getSelectedId() - 1;
    editor->setCurrentLayer(id);
}

void MainComponent::selectionChanged(juce::Component *m) {
    
}

void MainComponent::fileChanged(juce::String name) {
    lockButton->setToggleState(editor->isLocked(), juce::NotificationType::dontSendNotification);
    if (layerCombobox != NULL)
        layerCombobox->getComboBox().setSelectedId(editor->getCurrentLayer() + 1);
}

void MainComponent::dirtyChanged(bool dirty) {
    
}
