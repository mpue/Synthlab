/*
  ==============================================================================

    SynthlabAudioProcessor.h
    Created: 29 Apr 2018 9:41:01pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "SynthEditor.h"
#include "Mixer.h"

class SynthlabAudioProcessor : public AudioSource, public AudioProcessorPlayer {
    
public:
    
    SynthlabAudioProcessor();
    ~SynthlabAudioProcessor();
    
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;
    
    void handleIncomingMidiMessage (MidiInput* source, const MidiMessage& message) override;
    void sendGateMessage(Module* module, int channel, int velocity,int note,bool on);
    void sendNoteMessage(Module* module, int channel, int note);
    void sendControllerMessage(Module* module, int channel, int controller, float value);
    void processModule(Module* m);
    int getNumActiveChannels(int i);
    
    void setAudioChannels (int numInputChannels, int numOutputChannels, const XmlElement* const storedSettings = nullptr);
    SynthEditor* getEditor();
    
    void openSettings();    
    void shutdownAudio();
    int getCpuLoad();
    
    Mixer* getMixer();

private:
    // default settings just in case
    float sampleRate = 44100;
    float buffersize = 512;
    volatile bool running = false;
    SynthEditor* editor = nullptr;
    
    long currentTime = 0;
    long lastTime = 0;

    int cpuLoad = 0;
    
    Mixer* mixer = nullptr;
    
    AudioSourcePlayer audioSourcePlayer;
    AudioDeviceManager deviceManager;
    
    
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthlabAudioProcessor)
};
