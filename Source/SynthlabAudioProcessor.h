/*
  ==============================================================================

    SynthlabAudioProcessor.h
    Created: 18 May 2018 10:06:16am
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
#include "Metronome.h"
#include "SynthEditor.h"
#include "AudioEngine/Sampler.h"
#include "TransportDisplayComponent.h"
#include "Tracks/TrackNavigator.h"


#define _USE_MATH_DEFINES
#ifndef M_PI
#define M_PI       3.14159265358979323846 
#endif

inline float fastSin(float x) {
    // x ∈ [0, π/2] ideal!
    const float B = 4 / M_PI;
    const float C = -4 / (M_PI * M_PI);
    const float P = 0.225f;

    float y = B * x + C * x * std::fabs(x);
    return P * (y * std::fabs(y) - y) + y;
}

inline float fastCos(float x) {
    return fastSin((M_PI / 2.0f) - x);
}


class Module;
class SynthEditor;
class Sampler;
class Mixer;

class SynthlabAudioProcessor  {
    
public:
    
    SynthlabAudioProcessor(float sampleRate, int bufferSize, SynthEditor* editor, TransportDisplayComponent* transport, TrackNavigator* navigator);
    virtual ~SynthlabAudioProcessor() ;
    
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate);
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill);
    void releaseResources();
    int getNumActiveChannels(int i);
	void processModule(Module* m);
	Sampler* getDefaultSampler() { return defaultSampler; }
	int getCpuLoad() { return cpuLoad; }    
	void setCpuLoad(int load) { cpuLoad = load; }
	void setBouncing(bool b) { bouncing = b; }
	bool isBouncing() { return bouncing; }
	bool isRunning() { return running; }
	void setCurrentSample(int s) { currentSample = s; } 
        
private:

    // default settings just in case
    float sampleRate = 44100;
    float buffersize = 512;
    
    std::unique_ptr<Metronome> metronome;
    juce::AudioBuffer<float> metronomeBuffer;
    
    bool bouncing = false;
	bool running = false;
    
    long currentTime = 0;
    long lastTime = 0;
    int cpuLoad = 0;
    int currentSample = 0;

    Sampler* defaultSampler = nullptr;
	TransportDisplayComponent* transportDisplay = nullptr;
	TrackNavigator* navigator = nullptr;
	SynthEditor* editor = nullptr;
    
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthlabAudioProcessor)    

};
