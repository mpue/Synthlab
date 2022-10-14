/*
  ==============================================================================

    SamplerModule.cpp
    Created: 17 Apr 2018 3:34:36pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "SampleEditor.h"
#include "../JuceLibraryCode/JuceHeader.h"
#include "Connection.h"
#include "Project/Project.h"
#include "AudioManager.h"

using juce::AudioFormatManager;
using juce::AudioThumbnailCache;
using juce::AudioThumbnail;
using juce::AudioSampleBuffer;
using juce::Justification;
using juce::ImageCache;
using juce::Rectangle;
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
SamplerModule::SamplerModule(double sampleRate, int buffersize, AudioFormatManager* manager)
{
    this->sampleRate = sampleRate;
    this->buffersize = buffersize;
    for(int i = 0; i < 128;i++) {
        sampler[i] = nullptr;
    }

    cache = new AudioThumbnailCache(128);
    thumbnail = new AudioThumbnail(buffersize ,*manager,*cache);
    recordingBuffer = new AudioSampleBuffer(2,1024*1024);
    
    setSize(256,140);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(18,2);
    
    setName("Sampler");

    editable = false;
    prefab = true;
    
    selectSample(64);
    this->manager = manager;
    
    editor = new SampleEditor(this->buffersize, this->sampleRate, AudioManager::getInstance()->getFormatManager(), this);
}

SamplerModule::~SamplerModule()
{

    delete thumbnail;
    delete cache;
    delete recordingBuffer;
    
    for(int i = 0; i < 128;i++) {
        if (sampler[i] != nullptr) {
            delete sampler[i];
        }
    }
    
    delete editor;
    
}

void SamplerModule::loadSample(juce::InputStream* is, int samplerIndex) {
    
    sampler[samplerIndex]->loadSample(is);
    this->thumbnail->reset(2, sampleRate);
    thumbnail->addBlock(0, *sampler[samplerIndex]->getSampleBuffer(), 0, static_cast<int>(sampler[samplerIndex]->getSampleLength()));
    repaint();

}

void SamplerModule::loadSample(juce::File file, int samplerIndex) {

    sampler[samplerIndex]->loadSample(file);
    this->thumbnail->reset(2, sampleRate);
    thumbnail->addBlock(0, *sampler[samplerIndex]->getSampleBuffer(), 0, static_cast<int>(sampler[samplerIndex]->getSampleLength()));
    repaint();

}

void SamplerModule::configurePins() {
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

void SamplerModule::paint(juce::Graphics &g) {
    Module::paint(g);
    Rectangle<int> tb = Rectangle<int>(20,20,220,100);
    g.setColour(Colours::black);
    g.fillRect(tb);
    g.setColour(Colours::orange);
    this->thumbnail->drawChannels(g, tb,0,sampler[currentSampler]->getSampleLength() / sampleRate,1);

    g.setColour(juce::Colours::white);
    g.drawLine(samplePosX, 20,samplePosX, 120);

}



void SamplerModule::setAmplitude(float amplitude) {
    this->sampler[currentSampler]->setVolume(amplitude);
}

void SamplerModule::timerCallback() {
    samplePosX = (100.0f / sampler[currentSampler]->getSampleLength())* currentSample + 20;
    repaint();
}

void SamplerModule::process() {
    
    if (pins.at(4)->getConnections().size() == 1) {
        setPitch(pins.at(4)->getConnections().at(0)->getValue(),getCurrentSamplerIndex());
    }
    
    if (pins.at(0)->getConnections().size() == 1) {
        for (int i = 0; i < 128; i++) {
            if (sampler[i] != nullptr) {
                sampler[i]->setVolume(pins.at(0)->getConnections().at(0)->data[i]);
            }
        }
        
    }
    
    if (pins.at(1)->getAudioBuffer() != nullptr && pins.at(1)->getAudioBuffer()->getNumChannels() > 0){
        
        for (int j = 0; j < buffersize;j++) {
            
            float valueL = 0;
            float valueR = 0;
            
            for (int i = 0; i < 128;i++) {
                if (sampler[i] != nullptr) {
                    valueL +=  sampler[i]->getCurrentSample(0);
                    valueR +=  sampler[i]->getCurrentSample(1);
                 
                    if (!sampler[i]->isDone())  {
                        sampler[i]->nextSample();
                    }
                    
                }
                    
            }
        
            pins.at(1)->getAudioBuffer()->setSample(0,j ,valueL);
            pins.at(2)->getAudioBuffer()->setSample(0,j ,valueR);
        
            if (recording) {
                if (pins.at(5)->getConnections().size() == 1) {
                    recordingBuffer->setSample(0,  currentSample,pins.at(5)->getConnections().at(0)->getAudioBuffer()->getReadPointer(0)[currentSample%buffersize]);
                }
                if (pins.at(6)->getConnections().size() == 1) {
                    recordingBuffer->setSample(1,  currentSample,pins.at(6)->getConnections().at(0)->getAudioBuffer()->getReadPointer(0)[currentSample%buffersize]);
                }
                
                currentSample = (currentSample + 1) % (1024*1024);
                
                numRecordedSamples++;
            }
            
        }
    }


}

void SamplerModule::eventReceived(Event *e) {
    
    
    
    if (e->getType() == Event::Type::GATE) {
        if (e->getValue() > 0) {
            selectSample(e->getNote());
            if (sampler[e->getNote()] != nullptr) {
                sampler[e->getNote()]->reset();
            }
        }    
    }
    
}

void SamplerModule::setSamplePath(juce::String sample, int sampler) {
    this->samplePaths[sampler] = sample;
}

String SamplerModule::getSamplePath(int i) {
    return samplePaths[i];
}

void SamplerModule::setPitch(float pitch, int samplerIndex) {
    this->sampler[samplerIndex]->setPitch(pitch);
}

float SamplerModule::getPitch(int samplerIndex) {
    return sampler[samplerIndex]->getPitch();
}
void SamplerModule::setSampleRate(float rate) {
    this->sampleRate = rate;
}

void SamplerModule::setBuffersize(int buffersize){
    this->buffersize = buffersize;
}

bool SamplerModule::hasSampleAt(int i) {
    return (sampler[i] != nullptr && sampler[i]->getSampleLength() > 0);
}

AudioSampleBuffer* SamplerModule::getBuffer() {
    if (sampler[currentSampler] != nullptr && sampler[currentSampler]->getSampleLength() > 0) {
         return sampler[currentSampler]->getSampleBuffer();
    }
    return nullptr;
}

void SamplerModule::selectSample(int i) {

    
    currentSampler = i;
    
    if (sampler[currentSampler] == nullptr) {
        Sampler* sampler = new Sampler(sampleRate, buffersize);
        sampler->setVolume(0.5f);
        this->sampler[i] = sampler;
    }
    
    this->thumbnail->reset(2, sampleRate);
    if (sampler[currentSampler]->hasSample()) {
        thumbnail->addBlock(0, *sampler[currentSampler]->getSampleBuffer(), 0, static_cast<int>(sampler[currentSampler]->getSampleLength()));
    }
    std::function<void(void)> changeLambda =
    [=]() {  repaint(); };
    juce::MessageManager::callAsync(changeLambda);
}


void SamplerModule::startRecording() {
    
    if (!recording) {
        if (sampler[currentSampler] != nullptr) {
            sampler[currentSampler]->getSampleBuffer()->clear();
            recordingBuffer->clear();
            recording = true;
            currentSample = 0;
            numRecordedSamples = 0;
        }
        editor->getPanel()->startRecording();
    
    }
    
}


void SamplerModule::stopRecording() {
    if (recording) {
        recording = false;
        
        Logger::writeToLog("Recorded "+String(numRecordedSamples)+" samples.");
        
        sampler[currentSampler]->setDirty(true);

        WavAudioFormat* wavFormat = new WavAudioFormat();

        String userHome = File::getSpecialLocation(File::userHomeDirectory).getFullPathName();
        File appDir = File(userHome+"/.Synthlab");
        
        File dataDir = File(appDir);
        
        File outputFile = File(dataDir.getFullPathName()+"/"+String(Time::getMillisecondCounter())+".wav");
        setSamplePath(outputFile.getFullPathName(),currentSampler);
        
        Logger::writeToLog("wrote file "+outputFile.getFullPathName());
            
        std::unique_ptr<FileOutputStream> outputTo = outputFile.createOutputStream();
            
        AudioFormatWriter* writer = wavFormat->createWriterFor(outputTo.get(), sampleRate, 2, 16,NULL, 0);
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
       
        
        
    }
}
