/*
  ==============================================================================

    Sampler.h
    Created: 17 Apr 2018 1:12:15pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once


#include "../JuceLibraryCode/JuceHeader.h"
class Sampler  {
 
    
public:
    
    Sampler(float sampleRate, int bufferSize);
    ~Sampler();

    void play();
    void stop();
    float getSampleAt(int channel, long position);
    void loadSample(File file);
    void loadSample(InputStream* input);
    void setStartPosition(long start);
    long getStartPosition();
    void setEndPosition(long end);
    long getEndPosition();
    void setSampleLength(long length);
    long getSampleLength();
    void setLoop(bool loop);
    bool isLoop();
    void setVolume(float volume);
    AudioSampleBuffer* getSampleBuffer();
    bool hasSample();
    
private:
    
    AudioFormatManager* manager;
    
    float sampleRate;
    int bufferSize;
    
    float volume = 0.5;
    
    AudioSampleBuffer *sampleBuffer = nullptr;
    
    long currentSample = 0;
    
    long startPosition = 0;
    long endPosition = 0;
    long sampleLength = 0;
    
    bool loop = false;
    bool loaded = false;
};
