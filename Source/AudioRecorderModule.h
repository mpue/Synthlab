/*
  ==============================================================================

    AudioRecorderModule.h
    Created: 7 May 2018 2:17:45pm
    Author:  Matthias Pueski

  ==============================================================================
*/
#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"
#include "AudioRecorderEditor.h"

#pragma once

class AudioRecorderModule : public Module, public juce::Timer
{
public:
    AudioRecorderModule(double sampleRate, int buffersize, juce::AudioFormatManager* manager);
    ~AudioRecorderModule();
    
    void paint (juce::Graphics& g) override;
    void process() override;
    
    virtual void configurePins() override;
    virtual juce::String getCategory() override {
        return "Audio";
    }
    
    virtual void changeListenerCallback (ChangeBroadcaster* source) override;
    virtual void timerCallback() override;

    void loadSample(juce::InputStream* is, int sampler);
    void setSamplePath(juce::String sample, int sampler);
    juce::String getSamplePath(int i);
    
    juce::AudioSampleBuffer* getBuffer();
    
    bool isRecording() {
        return recording;
    }
    void startRecording();
    void stopRecording();
    
    AudioRecorderEditor* getEditor();
    
private:


    AudioRecorderEditor* editor = nullptr;
    
    virtual void setSampleRate(float rate) override;
    virtual void setBuffersize(int buffersize) override;
    
    juce::AudioSampleBuffer* recordingBuffer = nullptr;

    bool recording = false;
    
    int currentRecordingSample = 0;
    int currentPlaybackSample = 0;
    
    int numRecordedSamples = 0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioRecorderModule)
};
