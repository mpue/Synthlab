/*
  ==============================================================================

    SynthlabAudioProcessor.cpp
    Created: 29 Apr 2018 9:41:01pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "SynthlabAudioProcessor.h"
#include "AudioManager.h"
#include "Plugins/PluginManager.h"
#include "MidiNote.h"
#include "Knob.h"
#include "MidiGate.h"

//==============================================================================

SynthlabAudioProcessor::SynthlabAudioProcessor() {
    
    // specify the number of input and output channels that we want to open
    
    setAudioChannels (2, 2);
    
    AudioManager::getInstance()->setDeviceManager(&deviceManager);
    PluginManager::getInstance();
    
    mixer = new Mixer();
    editor = new SynthEditor(sampleRate,buffersize);    
    
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
    
    running = true;
    
}

SynthlabAudioProcessor::~SynthlabAudioProcessor() {

    running = false;
    
    editor->removeAllChangeListeners();
    
    for (int i = 0; i < MidiInput::getDevices().size();i++) {
        if (deviceManager.isMidiInputEnabled(MidiInput::getDevices().getReference(i))) {
            deviceManager.removeMidiInputCallback(MidiInput::getDevices().getReference(i),this);
        }
    }
    
    delete editor;
    
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

void SynthlabAudioProcessor::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    Logger::writeToLog("prepare to play with sample rate "+String(sampleRate)+" kHz and buffer size of "+String(buffersize)+" bytes.");
    
    this->sampleRate = sampleRate;
    this->buffersize = samplesPerBlockExpected;
    
    if (editor != nullptr) {
        editor->prepareToPlay(samplesPerBlockExpected, sampleRate);
    }
    
}

int SynthlabAudioProcessor::getNumActiveChannels(int i) {
    i = i - ((i >> 1) & 0x55555555);
    i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
    return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}


void SynthlabAudioProcessor::getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill)
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

void SynthlabAudioProcessor::releaseResources()
{
}


void SynthlabAudioProcessor::handleIncomingMidiMessage(juce::MidiInput *source, const juce::MidiMessage &message) {
    
    
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
    
    // deviceManager.getDefaultMidiOutput()->sendMessageNow(message);
}

void SynthlabAudioProcessor::sendGateMessage(Module *module,int channel, int velocity,int note,  bool on) {
    
    
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

void SynthlabAudioProcessor::sendNoteMessage(Module *module, int channel, int note) {
    
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

void SynthlabAudioProcessor::sendControllerMessage(Module *module, int channel, int controller, float value) {
    
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
#ifdef USE_PUSH
    SamplerModule* sm = nullptr;
    
    if ((sm = dynamic_cast<SamplerModule*>(module)) != NULL) {
        
        if (controller == 86 && value > 0) {
            if (sm->isRecording()) {
                sm->stopRecording();
            }
            else {
                sm->startRecording();
            }
            
        }
        else if (controller == 71) {
            // turn controller left
            if ((int)value & (1 << 6)) {
                long start = sm->getCurrentSampler()->getStartPosition();
                
                if (start - value < 0) {
                    sm->getCurrentSampler()->setStartPosition(0);
                }
                else {
                    sm->getCurrentSampler()->setStartPosition(start - value);
                }
                
                
            }
            else {
                long start = sm->getCurrentSampler()->getStartPosition();
                long end = sm->getCurrentSampler()->getSampleLength();
                
                if (sm->getCurrentSampler()->getStartPosition() + value * 100 >= end) {
                    sm->getCurrentSampler()->setStartPosition(end-2);
                }
                else {
                    sm->getCurrentSampler()->setStartPosition(start + value * 100);
                }
            }
            sm->updatePush2Display();
            
        }
        
        
    }
#endif
    
    for (int i = 0; i< module->getModules()->size();i++) {
        
        if ((k = dynamic_cast<Knob*>(module->getModules()->at(i)))!= NULL) {
            sendControllerMessage(module->getModules()->at(i), channel, controller, value);
        }
    }
}

void SynthlabAudioProcessor::processModule(Module* m) {
    
    if (m != nullptr && running) {
        
        m->process();
        
        
        for (int i = 0; i< m->getModules()->size();i++) {
            if (m->getModules()->at(i) != nullptr) {
                processModule(m->getModules()->at(i));
            }
        }
        
    }
    
}

void SynthlabAudioProcessor::setAudioChannels (int numInputChannels, int numOutputChannels, const XmlElement* const xml)
{
    String audioError = deviceManager.initialise (numInputChannels, numOutputChannels, xml, true);
    jassert (audioError.isEmpty());
    
    deviceManager.addAudioCallback (&audioSourcePlayer);
    audioSourcePlayer.setSource (this);
}

void SynthlabAudioProcessor::shutdownAudio()
{
    audioSourcePlayer.setSource (nullptr);
    deviceManager.removeAudioCallback (&audioSourcePlayer);
    deviceManager.closeAudioDevice();
}

SynthEditor* SynthlabAudioProcessor::getEditor() {
    return editor;
}


void SynthlabAudioProcessor::openSettings() {
    
    AudioDeviceSelectorComponent* selector = new AudioDeviceSelectorComponent(deviceManager, 2, 16, 2, 16, true, true, true, false);
    DialogWindow::LaunchOptions launchOptions;
    
    launchOptions.dialogTitle = ("Audio Settings");
    launchOptions.escapeKeyTriggersCloseButton = true;
    launchOptions.resizable = false;
    launchOptions.useNativeTitleBar = false;
    launchOptions.useBottomRightCornerResizer = true;
    launchOptions.componentToCentreAround = nullptr;
    launchOptions.content.setOwned(selector);
    launchOptions.content->setSize(600, 580);
    launchOptions.dialogBackgroundColour = LookAndFeel::getDefaultLookAndFeel().findColour(ResizableWindow::backgroundColourId);
    DialogWindow* window = launchOptions.launchAsync();
    
    std::function<void(int)> lambda =
    [=](int result) {
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
    };
    
    ModalComponentManager::Callback* callback = ModalCallbackFunction::create(lambda);
    ModalComponentManager::getInstance()->attachCallback(window, callback);
    
    Project::getInstance()->getOpenWindows().push_back(window);
    
}

Mixer* SynthlabAudioProcessor::getMixer() {
    return mixer;
}

int SynthlabAudioProcessor::getCpuLoad() {
    return cpuLoad;
}
