/*
  ==============================================================================

    AudioThumbnailComponent.h
    Created: 18 Apr 2018 3:40:25pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "push2/JuceToPush2DisplayBridge.h"

//==============================================================================
/*
*/
class AudioThumbnailComponent    : public Component
{
public:
    AudioThumbnailComponent(int buffersize, float sampleRate);
    ~AudioThumbnailComponent();

    void paint (Graphics&) override;
    void resized() override;

    void setSampleBuffer(AudioSampleBuffer* buffer);

private:
    
    AudioFormatManager* manager;
    AudioThumbnailCache* cache;
    AudioThumbnail* thumbnail;
    AudioSampleBuffer* buffer;
    
    float sampleRate;
    int buffersize;

    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioThumbnailComponent)
};
