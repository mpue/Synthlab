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
    
    if (this->thumbnail != nullptr && buffer != nullptr && buffer->getNumSamples() > 0){
        g.setColour(juce::Colours::orange);
        this->thumbnail->drawChannels(g, getBounds(), 0, this->buffer->getNumSamples() / sampleRate, 1);
    };
}

void AudioThumbnailComponent::resized()
{
    repaint();
}

void AudioThumbnailComponent::setSampleBuffer(AudioSampleBuffer *buffer) {
    
    this->buffer = buffer;
    this->thumbnail->reset(2, sampleRate);
    
    if (this->buffer != nullptr) {
        this->thumbnail->addBlock(0, *buffer, 0, buffer->getNumSamples());
    }
    std::function<void(void)> changeLambda =
    [=]() {  repaint(); };
    juce::MessageManager::callAsync(changeLambda);

}
