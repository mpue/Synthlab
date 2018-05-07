/*
 ==============================================================================
 
 AudioRecorderEditor
 Created: 7 May 2018 3:59:06pm
 Author:  Matthias Pueski
 
 ==============================================================================
 */

#include "../JuceLibraryCode/JuceHeader.h"
#include "AudioRecorderPanel.h"
#include "AudioEngine/Sampler.h"

class AudioRecorderEditor : public Component, public ChangeListener, public ChangeBroadcaster {
public:
    AudioRecorderEditor(float sampleRate, int bufferSize, AudioFormatManager* manager);
    ~AudioRecorderEditor();
    void paint(Graphics& g) override;
    void resized() override;
    Sampler* getSampler();
    void setNumSamples(int samples);
    AudioSampleBuffer* getBuffer();
    virtual void changeListenerCallback (ChangeBroadcaster* source) override;
    AudioRecorderPanel::State getState();
    void setState(AudioRecorderPanel::State state);
    void saveRecording();
private:
    AudioRecorderPanel* recorderPanel = nullptr;
    float sampleRate;
    int bufferSize;
    
    int numRecordedSamples = 0;
    
    juce::AudioFormatManager* manager;

    Sampler* sampler = nullptr;
    AudioSampleBuffer* recordingBuffer = nullptr;
    
    
};
