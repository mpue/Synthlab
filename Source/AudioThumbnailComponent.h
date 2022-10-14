/*
  ==============================================================================

    AudioThumbnailComponent.h
    Created: 18 Apr 2018 3:40:25pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

//==============================================================================
/*
*/
class AudioThumbnailComponent    : public juce::Component
{
public:
    AudioThumbnailComponent(int buffersize, float sampleRate);
    ~AudioThumbnailComponent();

    void paint (juce::Graphics&) override;
    void resized() override;

    void setSampleBuffer(juce::AudioSampleBuffer* buffer);
    void setStartPosition(int position);
    void setEndPosition(int position);
    void setCurrentPosition(int position);
    
private:
    
    juce::AudioFormatManager* manager;
    juce::AudioThumbnailCache* cache;
    juce::AudioThumbnail* thumbnail = nullptr;
    juce::AudioSampleBuffer* buffer = nullptr;
    
    float sampleRate;
    int buffersize;
    int startPosition;
    int endPosition;
    int currentPosition;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioThumbnailComponent)
};
