/*
  ==============================================================================

    SynthlabAudioProcessor.cpp
    Created: 18 May 2018 10:06:16am
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "SynthlabAudioProcessor.h"
#include "SynthEditor.h"
#include "Module.h"
#include "AudioEngine/Sampler.h"
#include "Mixer.h"
#include "MidiClock.h"
#include "MidiGate.h"
#include "PitchBendModule.h"
#include "MidiNote.h"
#include "Knob.h"

SynthlabAudioProcessor::SynthlabAudioProcessor(float sampleRate, int bufferSize, SynthEditor* editor, AudioDeviceManager& manager) : deviceManager(manager) {
    this->sampleRate = sampleRate;
    this->buffersize = bufferSize;
    this->editor = editor;
    // a global sampler object which allows us to play audio at any place like for preview for example
    defaultSampler = new Sampler(sampleRate, buffersize);
    Project::getInstance()->setDefaultSampler(defaultSampler);
}

SynthlabAudioProcessor::~SynthlabAudioProcessor() {
    delete defaultSampler;
}

//==============================================================================
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

int SynthlabAudioProcessor::getCpuLoad() {
    return cpuLoad;
}

void SynthlabAudioProcessor::getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill)
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

void SynthlabAudioProcessor::sendClockMessage(Module *module, int channel) {
    
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

void SynthlabAudioProcessor::sendMidiStartMessage(Module *module, int channel) {
    
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

void SynthlabAudioProcessor::sendMidiStopMessage(Module *module, int channel) {
    
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

void SynthlabAudioProcessor::sendPitchBendMessage(Module *module, int channel, int pitch) {
    
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

void SynthlabAudioProcessor::refreshMidiInputs() {
    for (int i = 0; i < MidiInput::getDevices().size();i++) {
        if (deviceManager.isMidiInputEnabled(MidiInput::getDevices().getReference(i))) {
            deviceManager.removeMidiInputCallback(MidiInput::getDevices().getReference(i),this);
            deviceManager.addMidiInputCallback(MidiInput::getDevices().getReference(i),this);
        }
    }
}

void SynthlabAudioProcessor::enableAllMidiInputs() {
    for (int i = 0; i < MidiInput::getDevices().size();i++) {
        if (deviceManager.isMidiInputEnabled(MidiInput::getDevices().getReference(i))) {
            deviceManager.addMidiInputCallback(MidiInput::getDevices().getReference(i),this);
        }
    }
}

void SynthlabAudioProcessor::disableAllMidiInputs() {
    for (int i = 0; i < MidiInput::getDevices().size();i++) {
        if (deviceManager.isMidiInputEnabled(MidiInput::getDevices().getReference(i))) {
            deviceManager.removeMidiInputCallback(MidiInput::getDevices().getReference(i),this);
        }
    }
}

