/*
  ==============================================================================

    SynthlabProcessorPlayer.h
    Created: 21 Apr 2018 3:58:49pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"
#include "SynthEditor.h"
class SynthlabProcessorPlayer : public AudioProcessorPlayer, public AudioAppComponent {
    
public:
    
    SynthlabProcessorPlayer();
    ~SynthlabProcessorPlayer();
    
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;
    void handleIncomingMidiMessage (MidiInput* source, const MidiMessage& message) override;
    void sendGateMessage(Module* module, int velocity,int note,bool on);
    void sendNoteMessage(Module* module, int note);
    void sendControllerMessage(Module* module, int controller, float value);
    void processModule(Module* m);
    void openSettings();
    SynthEditor* getEditor();
    
private:
    // default settings just in case
    float sampleRate = 44100;
    float buffersize = 512;
    SynthEditor* editor = nullptr;
    bool running = false;
};
