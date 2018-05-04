/*
  ==============================================================================

    MidiRecorder.h
    Created: 7 Apr 2018 12:53:35pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Logger.h"
#include <vector>
#include "RecordListener.h"

class MidiRecorder : public juce::MidiInputCallback, public juce::Timer {
    
public:
    
    MidiRecorder(MidiTools::Logger* logger);
    ~MidiRecorder();
    
    void handleIncomingMidiMessage (juce::MidiInput* source, const juce::MidiMessage& message) override;
    void timerCallback() override;
    
    bool isRecording();
    void startRecording();
    void stopRecording();
    
    bool isPlaying();
    void startPlaying();
    void stopPlaying();

    void setDeviceManager(juce::AudioDeviceManager* manager);
    void addListener(RecordListener* listener);
    void clear();
    
    juce::MidiFile* getFile();
    std::vector<juce::MidiMessage> getMessages();
    
private:
    
    MidiTools::Logger* logger;
    bool recording = false;
    bool playing = false;
    juce::MidiBuffer* midiBuffer;
    juce::AudioDeviceManager* deviceManager;
    double startTime = 0;
    int sampleRate = 44100;
    int previousSampleNumber = 0;
    std::vector<juce::MidiMessage> midiMessages;
    int currentMessageIndex = 0;
    std::vector<RecordListener*> listeners;
    
    void notifyListeners(RecordListener::StateChange* change);
};
