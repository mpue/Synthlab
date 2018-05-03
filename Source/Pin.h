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

    enum Direction{
        IN,
        OUT
    };
    
    enum Type {
        AUDIO,
        EVENT,
        VALUE
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
    
    float getValue();
    void setValue(float value);
    
    int getNote();
    void setNote(int note);
    
    void setTerminal(Pin* p) {
        this->terminal = p;
    }
    
    Pin* getTerminal() {
        return terminal;
    }
    
    void invalidate() {
        delete audioBuffer;
        audioBuffer = nullptr;
    }
    float value;
    float data[128] = {0};
    bool dataEnabled[128];
    bool hasConnection(Pin* target);
    
    virtual void process(const float *in, float *out,int numSamples) override;
    
private:
    bool selected = false;
    String name;
    int note = 0;
    Pin* terminal = nullptr;

    AudioSampleBuffer* audioBuffer = nullptr;
};
