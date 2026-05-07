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
#include "Graphics/Geometry.h"

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
    
	juce::Vector3D<float> getVector3D();
    void setVector3D(juce::Vector3D<float> vec);

	Geometry* getGeometry();
    void setGeometry(Geometry* geometry) {
        this->geometry = geometry;
	}

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

    // ---- Visual-debug helpers ----
    // Mark this pin as having just seen an event. Used by sendEvent and
    // anywhere a producer wants to flash its OUT pin without going through
    // the full sendEvent path. Cheap; safe to call from the audio thread.
    void noteEvent();

    // Sample the audio buffer for a peak and apply per-tick decay so the
    // glow falls off when the source goes silent. Called from the editor's
    // timer at ~30 Hz.
    void updateAudioPeak();

    // 0..1 fade based on time since the last event, over `fadeMs`.
    float getEventGlow(juce::int64 nowMs, juce::int64 fadeMs = 200) const;

    float getAudioLevel() const { return audioLevel; }

    float value = 0.0f;
    float data[128] = {0};
    bool dataEnabled[128] = {false};
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

    // Visual-debug state — accessed from audio thread (noteEvent /
    // updateAudioPeak) and from the message thread (paint). Plain types,
    // no locking; staleness by a frame is fine for visualisation.
    juce::int64 lastEventTimeMs = 0;
    float audioLevel = 0.0f;
    String description = "";
    juce::Vector3D<float> vector3D = { 0.0f, 0.0f, 0.0f };
	Geometry* geometry = nullptr;
};
