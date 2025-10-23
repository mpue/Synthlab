/*
  ==============================================================================

    GainModule.cpp
    Created: 22 Oct 2025 10:51:57am
    Author:  mpue

  ==============================================================================
*/

#include "GainModule.h"
#include "../JuceLibraryCode/JuceHeader.h"
#include "Connection.h"

using juce::Justification;
using juce::String;
using juce::ImageCache;

//==============================================================================
GainModule::GainModule(float sampleRate, int bufferSize)
{
    setSize(120, 140);
    setName("Gain");
    editable = false;
    prefab = true;
    this->buffersize = bufferSize;
    this->sampleRate = sampleRate;
}

GainModule::~GainModule()
{
}

void GainModule::configurePins() {
    // Audio Input Pin
    audioIn = new Pin(Pin::Type::AUDIO);
    audioIn->direction = Pin::Direction::IN;
    audioIn->setName("Audio In");
    
    // Gain Value Input Pin
    gainIn = new Pin(Pin::Type::VALUE);
    gainIn->direction = Pin::Direction::IN;
    gainIn->setName("Gain");
    
    // Audio Output Pin
    audioOut = new Pin(Pin::Type::AUDIO);
    audioOut->direction = Pin::Direction::OUT;
    audioOut->setName("Audio Out");
    
    addPin(Pin::Direction::IN, audioIn);
    audioIn->listeners.push_back(this);
    
    addPin(Pin::Direction::IN, gainIn);
    gainIn->listeners.push_back(this);
    
    addPin(Pin::Direction::OUT, audioOut);
    audioOut->listeners.push_back(this);
    
    audioOut->getAudioBuffer()->clear();
}

void GainModule::paint(juce::Graphics& g) {
    Module::paint(g);
    
    // Draw a simple gain icon (volume symbol)
    g.setColour(juce::Colours::white);
    
    // Draw speaker/volume icon
    int centerX = getWidth() / 2;
    int centerY = 70;
    
    // Speaker cone
    juce::Path speaker;
    speaker.addRectangle(centerX - 20, centerY - 10, 10, 20);
    speaker.addTriangle(centerX - 10, centerY - 15, 
                       centerX - 10, centerY + 15, 
                       centerX + 5, centerY + 10);
    speaker.addTriangle(centerX - 10, centerY - 15, 
                       centerX + 5, centerY - 10, 
                       centerX + 5, centerY + 10);
    
    // Sound waves
    for (int i = 1; i <= 3; i++) {
        juce::Path wave;
        wave.addArc(centerX - 20, centerY - 15 * i, 30 * i, 30 * i, 
                   -juce::MathConstants<float>::pi / 4, 
                   juce::MathConstants<float>::pi / 4, true);
        g.strokePath(wave, juce::PathStrokeType(2.0f));
    }
    
    g.fillPath(speaker);
}

void GainModule::process() {
    
    if (audioOut == nullptr || audioOut->getAudioBuffer() == nullptr) {
        return;
    }
    
    // Get output buffer
    float* pOut = audioOut->getAudioBuffer()->getWritePointer(0);
    
    // Default gain value
    float gainValue = 1.0f;
    
    // Get gain from input if connected
    if (gainIn != nullptr && gainIn->getConnections().size() > 0) {
        gainValue = gainIn->getConnections().at(0)->getValue();
    }
    
    // Process audio if connected
    if (audioIn != nullptr && audioIn->getConnections().size() > 0) {
        const float* pIn = audioIn->getConnections().at(0)->getAudioBuffer()->getReadPointer(0);
        
        // Apply gain to each sample
        for (int i = 0; i < buffersize; i++) {
            pOut[i] = pIn[i] * gainValue;
        }
    }
    else {
        // No input connected, output silence
        for (int i = 0; i < buffersize; i++) {
            pOut[i] = 0.0f;
        }
    }
}
