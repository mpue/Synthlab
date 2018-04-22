/*
  ==============================================================================

    SynthlabProcessorPlayer.cpp
    Created: 21 Apr 2018 3:58:49pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "SynthlabProcessorPlayer.h"
#include "MidiGate.h"
#include "Project.h"
#include "MidiNote.h"
#include "Knob.h"

SynthlabProcessorPlayer::SynthlabProcessorPlayer() {
    // specify the number of input and output channels that we want to open
    setAudioChannels (2, 2);
    editor = new SynthEditor();
    
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
    
    for (int i = 0; i < MidiInput::getDevices().size();i++) {
        if (deviceManager.isMidiInputEnabled(MidiInput::getDevices().getReference(i))) {
            deviceManager.addMidiInputCallback(MidiInput::getDevices().getReference(i),this);
        }
    }
    
    Project::getInstance()->setDeviceManager(&deviceManager);
    running = true;
}


SynthlabProcessorPlayer::~SynthlabProcessorPlayer() {
    for (int i = 0; i < MidiInput::getDevices().size();i++) {
        if (deviceManager.isMidiInputEnabled(MidiInput::getDevices().getReference(i))) {
            deviceManager.removeMidiInputCallback(MidiInput::getDevices().getReference(i),this);
        }
    }
    running = false;
    delete editor;
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

void SynthlabProcessorPlayer::handleIncomingMidiMessage(juce::MidiInput *source, const juce::MidiMessage &message) {
    
    
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

void SynthlabProcessorPlayer::sendGateMessage(Module *module,int velocity,int note,  bool on) {
    
    
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

void SynthlabProcessorPlayer::sendNoteMessage(Module *module, int note) {
    
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

void SynthlabProcessorPlayer::sendControllerMessage(Module *module, int controller, float value) {
    
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



void SynthlabProcessorPlayer::processModule(Module* m) {
    
    if (m != nullptr) {
        
        m->process();
        
        
        for (int i = 0; i< m->getModules()->size();i++) {
            if (m->getModules()->at(i) != nullptr) {
                processModule(m->getModules()->at(i));
            }
        }
        
    }
    
}

//==============================================================================
void SynthlabProcessorPlayer::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    Logger::writeToLog("prepare to play with sample rate "+String(sampleRate)+" kHz and buffer size of "+String(buffersize)+" bytes.");
    
    this->sampleRate = sampleRate;
    this->buffersize = samplesPerBlockExpected;
    
    if (editor != nullptr) {
        editor->prepareToPlay(samplesPerBlockExpected, sampleRate);
    }
    
}

void SynthlabProcessorPlayer::getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill)
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

void SynthlabProcessorPlayer::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.
    
    // For more details, see the help for AudioProcessor::releaseResources()
}

void SynthlabProcessorPlayer::openSettings() {
    
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

SynthEditor* SynthlabProcessorPlayer::getEditor() {
    return editor;
}
