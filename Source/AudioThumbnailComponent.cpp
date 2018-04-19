/*
  ==============================================================================

    AudioThumbnailComponent.cpp
    Created: 18 Apr 2018 3:40:25pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "AudioThumbnailComponent.h"
#include "Project.h"

//==============================================================================
AudioThumbnailComponent::AudioThumbnailComponent(int buffersize, float sampleRate)
{

    
    this->sampleRate = sampleRate;
    this->buffersize = buffersize;
    this->manager = Project::getInstance()->getFormatManager();
    this->cache = new AudioThumbnailCache(1);
    this->thumbnail = new AudioThumbnail(buffersize, *manager, *cache);
    setBounds(0,0,600,200);
    
}

AudioThumbnailComponent::~AudioThumbnailComponent()
{
    if (this->buffer == nullptr) {
        delete this->buffer;
    }
    delete this->thumbnail;
    delete this->cache;
}

void AudioThumbnailComponent::paint (Graphics& g)
{

    g.setColour(juce::Colours::black);
    g.fillRect(getBounds());
    g.setColour(juce::Colours::orange);
    
    if (this->thumbnail != nullptr && buffer != nullptr && buffer->getNumSamples() > 0){
        this->thumbnail->drawChannels(g, getBounds(), 0, this->buffer->getNumSamples() / sampleRate, 1);
        
    };
}

void AudioThumbnailComponent::resized()
{
    repaint();
}

void AudioThumbnailComponent::setSampleBuffer(AudioSampleBuffer *buffer) {
    

    this->buffer = buffer;
    /*
    if (this->buffer == nullptr) {
        delete this->buffer;
    }
    this->buffer = new AudioSampleBuffer(2,_buffer->getNumSamples());
    this->buffer->copyFrom(0,0, *_buffer,0,0,_buffer->getNumSamples());
    this->buffer->copyFrom(1,0, *_buffer,1,0,_buffer->getNumSamples());
     */
    this->thumbnail->reset(2, sampleRate);
    this->thumbnail->addBlock(0, *buffer, 0, buffer->getNumSamples());
    

    
    repaint();
}
