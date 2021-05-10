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
#include "Project/Project.h"
#include "AudioManager.h"
#include "AudioRecorderEditor.h"

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
    delete editor;
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

	Pin* p5 = new Pin(Pin::Type::EVENT);
	p5->direction = Pin::Direction::IN;
	p5->listeners.push_back(this);
	p5->setName("Tr");


    addPin(Pin::Direction::IN,p1);
    addPin(Pin::Direction::IN,p2);
    addPin(Pin::Direction::OUT,p3);
    addPin(Pin::Direction::OUT,p4);
	addPin(Pin::Direction::IN, p5);
}

void AudioRecorderModule::paint(juce::Graphics &g) {
    Module::paint(g);
}


void AudioRecorderModule::timerCallback() {
    if (tick % 50 == 0) {
        if (recording) {
            updateRecordingTime();
            seconds++;
        }
    }
    tick++;
    editor->setMagnitude(0, magnitudeL);
    editor->setMagnitude(1, magnitudeR);
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
        inR = getPins().at(1)->getConnections().at(0)->getAudioBuffer()->getReadPointer(0);
    }

    outL = getPins().at(2)->getAudioBuffer()->getWritePointer(0);
    outR = getPins().at(3)->getAudioBuffer()->getWritePointer(0);

    if (editor->getState() == AudioRecorderPanel::State::RECORDING) {
        
        for (int i = 0; i < buffersize;i++) {
        
            if (inL != nullptr) {
                editor->getBuffer()->setSample(0, currentRecordingSample,inL[i] * editor->getGain());
				outL[i] = inL[i] * editor->getGain();
            }
			else {
				editor->getBuffer()->setSample(0, currentRecordingSample, 0);
				outL[i] = 0;
			}
            
            if (inR != nullptr) {
                editor->getBuffer()->setSample(1, currentRecordingSample,inR[i] * editor->getGain());
				outR[i] = inR[i] * editor->getGain();	
            }
			else {
				editor->getBuffer()->setSample(1, currentRecordingSample, 0);
				outR[i] = 0;
			}
        

            currentRecordingSample = (currentRecordingSample + 1);
            numRecordedSamples++;
            
            editor->setNumSamples(numRecordedSamples);
        }
        if (currentRecordingSample > buffersize) {
            magnitudeL = editor->getBuffer()->getMagnitude(0, currentRecordingSample - buffersize, buffersize) * editor->getGain();
            magnitudeR = editor->getBuffer()->getMagnitude(1, currentRecordingSample - buffersize, buffersize) * editor->getGain();
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
            if (inL != nullptr) {
                outL[i] = inL[i];
                magnitudeL =  getPins().at(0)->getConnections().at(0)->getAudioBuffer()->getMagnitude(0, 0, buffersize) * editor->getGain();
            }
			else {
				outL[i] = 0;
			}
            
            if (inR != nullptr) {
                outR[i] = inR[i];
                magnitudeR = getPins().at(1)->getConnections().at(0)->getAudioBuffer()->getMagnitude(0, 0, buffersize) * editor->getGain();
            }
			else {
				outR[i] = 0;
			}
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
            recording = true;
            if (!isTimerRunning()) {
                startTimer(20);
            }
            currentRecordingSample = 0;
            numRecordedSamples = 0;
            seconds = 0;
        }
    }

    
}


void AudioRecorderModule::stopRecording() {
    if (recording) {
        recording = false;
    }
}

AudioRecorderEditor* AudioRecorderModule::getEditor() {
    return editor;
}

void AudioRecorderModule::changeListenerCallback (ChangeBroadcaster* source) {
    
    if (source == editor) {
     
        if (editor->getState() == AudioRecorderPanel::State::RECORDING) {
            startRecording();
        }
        if (editor->getState() == AudioRecorderPanel::State::PLAYING) {
            
        }
        else if (editor->getState() == AudioRecorderPanel::State::IDLE) {
            stopRecording();
        }
        else if (editor->getState() == AudioRecorderPanel::State::SAVE) {
            
        }
        if (editor->isMonitoring()) {
            startTimer(20);
        }
        else {
            if (!recording) {
                stopTimer();
            }
        }
    }
    
}


void AudioRecorderModule::updateRecordingTime() {
    
    String time;
    
    int _minutes = seconds / 60;
    // int _seconds = seconds % 60;
    
    time = String(_minutes)+":";
    
    if (seconds < 10) {
        time += "0";
    }
    
    time += String(seconds);
    editor->setCurrentTime(time);
     
}

void AudioRecorderModule::eventReceived(Event *e) {
	if (e->getType() == Event::Type::GATE) {

		if (e->getValue() > 0) {
			getEditor()->toggleRecording();
		}

	}
	if (e->getType() == Event::Type::CLOCK_START) {
		getEditor()->toggleRecording();
	}
}
