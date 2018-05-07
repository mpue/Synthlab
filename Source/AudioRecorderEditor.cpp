/***********************************************************************
 * AUTHOR: Matthias Pueski <mpue>
 *   FILE: .//AudioRecorderEditor.cpp
 *   DATE: Mon May  7 17:38:27 2018
 *  DESCR: 
 ***********************************************************************/
#include "AudioRecorderEditor.h"

/*
 *  Method: AudioRecorderEditor::AudioRecorderEditor()
 *   Descr: 
 */
AudioRecorderEditor::AudioRecorderEditor(float sampleRate, int bufferSize , AudioFormatManager* manager) {
    this->sampleRate = sampleRate;
    this->bufferSize = bufferSize;
    recordingBuffer = new AudioSampleBuffer(2,10204*1024);
    sampler = new Sampler(sampleRate, bufferSize);
    this->manager = manager;
    this->recorderPanel = new AudioRecorderPanel(sampleRate, bufferSize);
    addAndMakeVisible(recorderPanel);
    this->recorderPanel->addChangeListener(this);
}


/*
 *  Method: AudioRecorderEditor::~AudioRecorderEditor()
 *   Descr: 
 */
AudioRecorderEditor::~AudioRecorderEditor() {
    delete recordingBuffer;
    delete sampler;
    this->recorderPanel->removeAllChangeListeners();
    delete recorderPanel;
}



/*
 *  Method: AudioRecorderEditor::resized()
 *   Descr: 
 */
void AudioRecorderEditor::resized() {
    if (getParentComponent() != nullptr) {
        setSize(getParentWidth(), getParentHeight());
    }
}

void AudioRecorderEditor::paint(juce::Graphics& g) {
}


void AudioRecorderEditor::changeListenerCallback(juce::ChangeBroadcaster *source) {
    AudioRecorderPanel* panel = dynamic_cast<AudioRecorderPanel*>(source);
    
    if (panel != nullptr) {
        if (panel->getState() == AudioRecorderPanel::State::RECORDING) {
            sampler->setStartPosition(0);
            sampler->setLoaded(false);
            sampler->reset();
            sendChangeMessage();
        }
        else if (panel->getState() == AudioRecorderPanel::State::IDLE) {
            panel->updateContent(getBuffer(), static_cast<int>(numRecordedSamples));
            sampler->setEndPosition(numRecordedSamples);
            sampler->setSampleLength(numRecordedSamples);
            sampler->setStartPosition(0);
            sampler->reset();
            sendChangeMessage();
        }
        else if (panel->getState() == AudioRecorderPanel::State::PLAYING) {
            sendChangeMessage();
        }
        else if (panel->getState() == AudioRecorderPanel::State::SAVE) {
            sendChangeMessage();
        }
    }
    
}

Sampler* AudioRecorderEditor::getSampler() {
    return sampler;
}

AudioRecorderPanel::State AudioRecorderEditor::getState() {
    return recorderPanel->getState();
}

void AudioRecorderEditor::setState(AudioRecorderPanel::State state) {
    recorderPanel->setState(state);
}

AudioSampleBuffer* AudioRecorderEditor::getBuffer() {
    return sampler->getSampleBuffer();
}

void AudioRecorderEditor::setNumSamples(int samples) {
    this->numRecordedSamples = samples;
}
