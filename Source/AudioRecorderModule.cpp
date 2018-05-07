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

    
    cache = new AudioThumbnailCache(128);
    thumbnail = new AudioThumbnail(buffersize ,*manager,*cache);
    recordingBuffer = new AudioSampleBuffer(2,1024*1024);
    
    setSize(256,140);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(18,2);
    
    setName("Sampler");
    
    editable = false;
    prefab = true;
    
    sampler = new Sampler(sampleRate, buffersize);
    this->manager = manager;
    

}

AudioRecorderModule::~AudioRecorderModule()
{
    
    delete thumbnail;
    delete cache;
    delete recordingBuffer;
    delete sampler;

}



void AudioRecorderModule::configurePins() {
    Pin* p1 = new Pin(Pin::Type::VALUE);
    p1->direction = Pin::Direction::IN;
    p1->listeners.push_back(this);
    p1->setName("A");
    
    Pin* p2 = new Pin(Pin::Type::AUDIO);
    p2->direction = Pin::Direction::OUT;
    p2->listeners.push_back(this);
    p2->setName("L");
    
    Pin* p3 = new Pin(Pin::Type::AUDIO);
    p3->direction = Pin::Direction::OUT;
    p3->listeners.push_back(this);
    p3->setName("R");
    
    Pin* p4 = new Pin(Pin::Type::EVENT);
    p4->direction = Pin::Direction::IN;
    p4->listeners.push_back(this);
    p4->setName("E");
    
    Pin* p5 = new Pin(Pin::Type::VALUE);
    p5->direction = Pin::Direction::IN;
    p5->listeners.push_back(this);
    p5->setName("P");
    
    
    Pin* p6 = new Pin(Pin::Type::AUDIO);
    p6->direction = Pin::Direction::IN;
    p6->listeners.push_back(this);
    p6->setName("L");
    
    
    Pin* p7 = new Pin(Pin::Type::AUDIO);
    p7->direction = Pin::Direction::IN;
    p7->listeners.push_back(this);
    p7->setName("R");
    
    addPin(Pin::Direction::IN,p1);
    addPin(Pin::Direction::OUT,p2);
    addPin(Pin::Direction::OUT,p3);
    addPin(Pin::Direction::IN,p4);
    addPin(Pin::Direction::IN,p5);
    addPin(Pin::Direction::IN,p6);
    addPin(Pin::Direction::IN,p7);
    
}

void AudioRecorderModule::paint(juce::Graphics &g) {
    Module::paint(g);
}


void AudioRecorderModule::timerCallback() {

}

void AudioRecorderModule::process() {
    
    
}




void AudioRecorderModule::setSampleRate(float rate) {
    this->sampleRate = rate;
}

void AudioRecorderModule::setBuffersize(int buffersize){
    this->buffersize = buffersize;
}


void AudioRecorderModule::startRecording() {
    
    if (!recording) {
        if (sampler != nullptr) {
            sampler->getSampleBuffer()->clear();
            recordingBuffer->clear();
            recording = true;
            currentSample = 0;
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

