/*
  ==============================================================================

    MidiRecorder.cpp
    Created: 7 Apr 2018 12:53:35pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "MidiRecorder.h"
#include <vector>
MidiRecorder::MidiRecorder(MidiTools::Logger* logger) {
    this->logger = logger;

}

MidiRecorder::~MidiRecorder() {
    
}

void MidiRecorder::handleIncomingMidiMessage(juce::MidiInput *source, const juce::MidiMessage &message) {
    //Logger->log(message.getDescription());

    MidiMessage m = message.withTimeStamp(Time::getMillisecondCounterHiRes() * 0.001 - startTime);
    
    if (recording) {
        midiMessages.push_back(MidiMessage(m));
    }
    deviceManager->getDefaultMidiOutput()->sendMessageNow(message);
    
    for (int i = 0; i < listeners.size();i++) {
        listeners.at(i)->incomingMessage(&m);
    }
    
}

void MidiRecorder::timerCallback() {
    
    
    if (midiMessages.size() > 0 && midiMessages.at(currentMessageIndex).getTimeStamp() <= Time::getMillisecondCounterHiRes() * 0.001 - startTime) {
        deviceManager->getDefaultMidiOutput()->sendMessageNow(midiMessages.at(currentMessageIndex));
        
        if (currentMessageIndex < midiMessages.size() - 1) {
            currentMessageIndex++;
        }
        else {
            currentMessageIndex = 0;
            stopPlaying();
        }
    
    }

}

bool MidiRecorder::isRecording() {
    return recording;
}

void MidiRecorder::stopRecording() {
    recording = false;
    RecordListener::StateChange change;
    change.newState = RecordListener::RECORDING_STOPPED;
    change.message = "Recorded "+String(midiMessages.size())+ " event(s)";
    notifyListeners(&change);
}

void MidiRecorder::startRecording(){
    midiMessages.clear();
    recording = true;
    startTime = Time::getMillisecondCounterHiRes() * 0.001;
    RecordListener::StateChange change;
    change.newState = RecordListener::RECORDING_STARTED;
    
    notifyListeners(&change);
}

bool MidiRecorder::isPlaying(){
    return playing;
}

void MidiRecorder::stopPlaying(){
    deviceManager->getDefaultMidiOutput()->sendMessageNow(MidiMessage::allNotesOff(1));
    stopTimer();
    playing = false;
    
    RecordListener::StateChange change;
    change.newState = RecordListener::PLAYING_STOPPED;
    
    notifyListeners(&change);
}

void MidiRecorder::startPlaying(){
    playing = true;
    startTime = Time::getMillisecondCounterHiRes() * 0.001;
    startTimer(120/(60*64));
    currentMessageIndex = 0;
    
    RecordListener::StateChange change;
    change.newState = RecordListener::PLAYING_STARTED;
    
    notifyListeners(&change);
    
}

void MidiRecorder::setDeviceManager(juce::AudioDeviceManager *manager) {
    this->deviceManager = manager;
}

void MidiRecorder::addListener(RecordListener *listener) {
    listeners.push_back(listener);
}

void MidiRecorder::notifyListeners(RecordListener::StateChange *change) {
    for (int i = 0; i < listeners.size();i++) {
        listeners.at(i)->changedState(change);
    }
}

void MidiRecorder::clear() {
    midiMessages.clear();
}

MidiFile* MidiRecorder::getFile() {
    MidiFile* midiFile = new MidiFile();
    
    MidiMessageSequence mms;
    
    for (int i = 0; i < midiMessages.size();i++) {
        mms.addEvent(midiMessages.at(i));
    }
    
    midiFile->addTrack(mms);
    return midiFile;
}

std::vector<MidiMessage> MidiRecorder::getMessages() {
    return midiMessages;
}
