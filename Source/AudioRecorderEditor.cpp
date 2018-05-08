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
    
    setSize(1000, 200);
    this->sampleRate = sampleRate;
    this->bufferSize = bufferSize;
    recordingBuffer = new AudioSampleBuffer(2,10204*1024);
    sampler = new Sampler(sampleRate, bufferSize);
    this->manager = manager;
    this->recorderPanel = new AudioRecorderPanel(sampleRate, bufferSize);
    addAndMakeVisible(recorderPanel);
    this->recorderPanel->addChangeListener(this);
    recorderPanel->setTopLeftPosition(0,8);

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
    
    

    //setSize(proportionOfWidth (1.0000f),proportionOfHeight(1.0000f));
    /*
    if (getParentComponent() != nullptr) {
        setSize(getParentWidth(), getParentHeight());
    }
     */
}

void AudioRecorderEditor::paint(juce::Graphics& g) {
}


void AudioRecorderEditor::changeListenerCallback(juce::ChangeBroadcaster *source) {
    AudioRecorderPanel* panel = dynamic_cast<AudioRecorderPanel*>(source);
    
    if (panel != nullptr) {
        if (panel->getState() == AudioRecorderPanel::State::RECORDING) {
            numRecordedSamples = 0;
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
            panel->setState(AudioRecorderPanel::State::IDLE);
            saveRecording();
            sendChangeMessage();
        }
        
        if (panel->isMonitoring()) {
            sendChangeMessage();
        }
    }
    
}

void AudioRecorderEditor::saveRecording() {
    WavAudioFormat* wavFormat = new WavAudioFormat();
    
    String userHome = File::getSpecialLocation(File::userHomeDirectory).getFullPathName();
    File appDir = File(userHome+"/.Synthlab");
    
    FileChooser chooser("Select file to save", File::nonexistent, "*");
    
    if (chooser.browseForFileToSave(true)) {
        File outputFile = chooser.getResult();
        FileOutputStream* outputTo = outputFile.createOutputStream();
        AudioFormatWriter* writer = wavFormat->createWriterFor(outputTo, sampleRate, 2, 16,NULL, 0);
        writer->writeFromAudioSampleBuffer(*getSampler()->getSampleBuffer(), 0,numRecordedSamples);
        delete writer;
        delete wavFormat;

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

void AudioRecorderEditor::setCurrentTime(juce::String time) {
    this->recorderPanel->setCurrentTime(time);
    recorderPanel->updateContent(getBuffer(), static_cast<int>(numRecordedSamples));
}

void AudioRecorderEditor::setMagnitude(int channel, float value) {
    recorderPanel->setMagnitude(channel, value);
}

bool AudioRecorderEditor::isMonitoring() {
    return recorderPanel->isMonitoring();
}

float AudioRecorderEditor::getGain() {
    return recorderPanel->getGain();
}
