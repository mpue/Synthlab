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
    this->buffersize = buffersize;
    this->manager = Project::getInstance()->getFormatManager();
    this->cache = new AudioThumbnailCache(1);
    this->thumbnail = new AudioThumbnail(buffersize, *manager, *cache);
    
}

AudioThumbnailComponent::~AudioThumbnailComponent()
{
}

void AudioThumbnailComponent::paint (Graphics& g)
{
    if (this->thumbnail != nullptr){
        this->thumbnail->drawChannel(g, getBounds(), 0, <#double endTimeSeconds#>, 0, 1);
        
    };
}

void AudioThumbnailComponent::resized()
{

}

void AudioThumbnailComponent::setSampleBuffer(AudioSampleBuffer *buffer) {
    this->buffer = buffer;
    this->thumbnail->addBlock(0, *buffer, 0, buffer->getNumSamples());
}
