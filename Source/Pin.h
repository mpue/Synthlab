/*
  ==============================================================================

    Pin.h
    Created: 1 Apr 2018 6:49:53pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once

#include "AudioEngine/Event.h"
#include "AudioEngine/EventListener.h"
#include "../JuceLibraryCode/JuceHeader.h"
#include "AudioEngine/AudioProcessorModule.h"
#include "Terminal.h"

class Pin : public juce::ChangeBroadcaster,
            public AudioProcessorModule,
            public Terminal,
            public juce::ChangeListener {


public:
    
    Pin(Terminal::Type type);
    ~Pin();
    
    int x;
    long index;
    int y;

    Terminal::Direction direction;
    
    void setSelected(bool selected);
    bool isSelected();
    
    juce::AudioSampleBuffer* getAudioBuffer();
    void setAudioBuffer(juce::AudioSampleBuffer* buffer);
    
    std::vector<Pin*>& getConnections ();
                
    std::vector<EventListener*> listeners;
    
    void sendEvent(Event* e);
    
    juce::String getName();
    void setName(juce::String name);
    
    float getValue();
    void setValue(float value);
    
    int getNote();
    void setNote(int note);
                
    virtual juce::String getDescription() {
        return description;
    };
                
    void setDescription(juce::String description) {
        this->description = description;
    }
    
    void setTerminal(Pin* p) {
        this->terminal = p;
    }
    
    Pin* getTerminal() {
        return terminal;
    }
                
    Terminal::Direction getDirection() {
        return direction;
    }
                
    void invalidate() {
        delete audioBuffer;
        audioBuffer = nullptr;
    }
                
    void addEventListener(EventListener* listener);
                
    float value;
    float data[128] = {0};
    bool dataEnabled[128];
    bool hasConnection(Pin* target);
    
                
    virtual void process(const float *in, float *out,int numSamples) override;
    void changeListenerCallback (juce::ChangeBroadcaster* source) override;
private:
    bool selected = false;
    juce::String name;
    int note = 0;
    Pin* terminal = nullptr;
    std::vector<Pin*> connections;
    juce::AudioSampleBuffer* audioBuffer = nullptr;
    String description = "";
};
