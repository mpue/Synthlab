/*
  ==============================================================================

    AudioRecorderModule.cpp
    Created: 7 May 2018 2:17:45pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "AudioRecorderModule.h"

#include "../JuceLibraryCode/JuceHeader.h"
#include "Connection.h"
#include "Project.h"
#include "AudioManager.h"


using juce::AudioFormatManager;
using juce::AudioThumbnailCache;
using juce::AudioThumbnail;
using juce::AudioSampleBuffer;
using juce::Justification;
using juce::ImageCache;
using juce::Rectangle;
using juce::Colours;
using juce::String;
using juce::Logger;
using juce::WavAudioFormat;
using juce::File;
using juce::Timer;
using juce::FileOutputStream;
using juce::AudioFormatWriter;
using juce::AudioDeviceManager;
using juce::MidiMessage;
using juce::Time;

//==============================================================================
AudioRecorderModule::AudioRecorderModule(double sampleRate, int buffersize, AudioFormatManager* manager)
{
    this->sampleRate = sampleRate;
    this->buffersize = buffersize;
    
    setSize(120,140);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(18,2);
    
    setName("Recorder");
    
    editable = false;
    prefab = true;
    
    this->editor = new AudioRecorderEditor(sampleRate, buffersize, manager);
    editor->addChangeListener(this);
}

AudioRecorderModule::~AudioRecorderModule()
{
    editor->removeAllChangeListeners();
    // delete editor;
}



void AudioRecorderModule::configurePins() {

    Pin* p1 = new Pin(Pin::Type::AUDIO);
    p1->direction = Pin::Direction::IN;
    p1->listeners.push_back(this);
    p1->setName("L");
    
    Pin* p2 = new Pin(Pin::Type::AUDIO);
    p2->direction = Pin::Direction::IN;
    p2->listeners.push_back(this);
    p2->setName("R");
    
    Pin* p3 = new Pin(Pin::Type::AUDIO);
    p3->direction = Pin::Direction::OUT;
    p3->listeners.push_back(this);
    p3->setName("L");
    
    
    Pin* p4 = new Pin(Pin::Type::AUDIO);
    p4->direction = Pin::Direction::OUT;
    p4->listeners.push_back(this);
    p4->setName("R");
    
    addPin(Pin::Direction::IN,p1);
    addPin(Pin::Direction::IN,p2);
    addPin(Pin::Direction::OUT,p3);
    addPin(Pin::Direction::OUT,p4);
    
}

void AudioRecorderModule::paint(juce::Graphics &g) {
    Module::paint(g);
}


void AudioRecorderModule::timerCallback() {

}

void AudioRecorderModule::process() {
    
    const float* inL = nullptr;
    const float* inR = nullptr;
    
    float* outL = nullptr;
    float* outR = nullptr;
    
    if (getPins().at(0)->getConnections().size() >= 1) {
        inL = getPins().at(0)->getConnections().at(0)->getAudioBuffer()->getReadPointer(0);
    }
    if (getPins().at(1)->getConnections().size() >= 1) {
        inR = getPins().at(1)->getConnections().at(1)->getAudioBuffer()->getReadPointer(0);
    }

    outL = getPins().at(2)->getAudioBuffer()->getWritePointer(0);
    outR = getPins().at(3)->getAudioBuffer()->getWritePointer(0);

    if (editor->getState() == AudioRecorderPanel::State::RECORDING) {
        
        for (int i = 0; i < buffersize;i++) {
        
            if (inL != nullptr)
                editor->getBuffer()->setSample(0, currentRecordingSample,inL[i]);
            
            if (inR != nullptr)
                editor->getBuffer()->setSample(1, currentRecordingSample,inR[i]);
        
            currentRecordingSample = (currentRecordingSample + 1);
            numRecordedSamples++;
            
            editor->setNumSamples(numRecordedSamples);
        }
    }
    else if (editor->getState() == AudioRecorderPanel::State::PLAYING) {
        
        for (int i = 0; i < buffersize;i++) {
            editor->getSampler()->nextSample();
            currentPlaybackSample = (currentPlaybackSample + 1) % numRecordedSamples;
            outL[i] = editor->getSampler()->getCurrentSample(0);
            outR[i] = editor->getSampler()->getCurrentSample(1);

        }
        

    }
    else {
        for (int i = 0; i < buffersize;i++) {
            if (inL != nullptr)
                outL[i] = inL[i];
            if (inR != nullptr)
                outR[i] = inR[i];
        }
    }
    

}




void AudioRecorderModule::setSampleRate(float rate) {
    this->sampleRate = rate;
}

void AudioRecorderModule::setBuffersize(int buffersize){
    this->buffersize = buffersize;
}


void AudioRecorderModule::startRecording() {
    

    if (!recording) {
        if (editor->getSampler() != nullptr) {
            editor->getSampler()->getSampleBuffer()->clear();
            recordingBuffer->clear();
            recording = true;
            currentRecordingSample = 0;
            numRecordedSamples = 0;
        }
    }

    
}


void AudioRecorderModule::stopRecording() {
    if (recording) {
        recording = false;
        /*
        Logger::writeToLog("Recorded "+String(numRecordedSamples)+" samples.");
        
        sampler[currentSampler]->setDirty(true);
        
        WavAudioFormat* wavFormat = new WavAudioFormat();
        
        String userHome = File::getSpecialLocation(File::userHomeDirectory).getFullPathName();
        File appDir = File(userHome+"/.Synthlab");
        
        File dataDir = File(appDir);
        
        File outputFile = File(dataDir.getFullPathName()+"/"+String(Time::getMillisecondCounter())+".wav");
        setSamplePath(outputFile.getFullPathName(),currentSampler);
        
        Logger::writeToLog("wrote file "+outputFile.getFullPathName());
        
        FileOutputStream* outputTo = outputFile.createOutputStream();
        
        AudioFormatWriter* writer = wavFormat->createWriterFor(outputTo, sampleRate, 2, 16,NULL, 0);
        writer->writeFromAudioSampleBuffer(*recordingBuffer, 0,numRecordedSamples);
        delete writer;
        delete wavFormat;
        
        this->sampler[currentSampler]->loadSample(outputFile);
        
        
        if (sampler[currentSampler]->hasSample()) {
            selectSample(currentSampler);
            
            thumbnail->addBlock(0, *recordingBuffer, 0, numRecordedSamples);
            std::function<void(void)> changeLambda =
            [=]() {  repaint(); };
            juce::MessageManager::callAsync(changeLambda);
            
        }
        editor->getPanel()->stopRecording();
        
        
        */
    }
}

AudioRecorderEditor* AudioRecorderModule::getEditor() {
    return editor;
}

void AudioRecorderModule::changeListenerCallback (ChangeBroadcaster* source) {
    
    if (source == editor) {
     
        if (editor->getState() == AudioRecorderPanel::State::RECORDING) {
            
        }
        if (editor->getState() == AudioRecorderPanel::State::PLAYING) {
            
        }
        else if (editor->getState() == AudioRecorderPanel::State::IDLE) {
            
        }
        else if (editor->getState() == AudioRecorderPanel::State::SAVE) {
            
        }

    }
    
}
