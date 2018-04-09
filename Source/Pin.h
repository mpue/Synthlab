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

class Pin : public ChangeBroadcaster, public AudioProcessorModule {


public:

    static enum Direction{
        IN,
        OUT
    };
    
    static enum Type {
        AUDIO,
        EVENT
    };
    
    Pin(Type type);
    ~Pin();
    
    int x;
    long index;
    int y;

	Direction direction;
    Type type;
    
    void setSelected(bool selected);
    bool isSelected();
    
    AudioSampleBuffer* getAudioBuffer();
    
    std::vector<Pin*> connections;
    std::vector<EventListener*> listeners;
    
    void sendEvent(Event* e);
    
    String getName();
    void setName(String name);
    
    virtual void process(float *in, float *out,int numSamples) override;
    
private:
    bool selected = false;
    String name;
    AudioSampleBuffer* audioBuffer = nullptr;
};
