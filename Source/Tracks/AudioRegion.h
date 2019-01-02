/*
  ==============================================================================

    Track.h
    Created: 25 Nov 2016 10:00:36pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#ifndef AUDIOREGION_H_INCLUDED 
#define AUDIOREGION_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"
#include "MultiComponentDragger.h"
#include "../ResizeConstrainer.h"
#include "Region.h"


//==============================================================================
/*
*/
class AudioRegion : public Region
{
public:
    
    AudioRegion(AudioRegion* other, AudioFormatManager& manager, double sampleRate, long startSample, long numSamples);
    AudioRegion(AudioRegion* other, AudioFormatManager& manager, double sampleRate);
    AudioRegion(AudioSampleBuffer* source, AudioFormatManager& manager, long startSample, long sampleLength, double sampleRate);
    AudioRegion(File file, String refId, AudioFormatManager& manager,double sampleRate);
    ~AudioRegion();

    virtual void paint (Graphics&) override;
    virtual void resized() override;

    AudioThumbnail* getThumbnail();
    virtual void setThumbnailBounds(Rectangle<int>* bounds) override;
    virtual Rectangle<int>* getThumbnailBounds() override;
    
    virtual int getNumSamples() override;
    
    AudioSampleBuffer* getBuffer();
    const float* getReadBuffer(int channel);    

    virtual void setZoom(float zoom) override;
    virtual void setLoop(bool loop) override;
    
    // the timer is needed to update the thumbnail during record
    virtual void timerCallback() override;
    virtual void startRecording() override;
    virtual void stopRecording() override;
    
    // component dragger used to drag and drop regions
    void setDragger(MultiComponentDragger* dragger);
    
    virtual void updateThumb() override;
    virtual void componentMovedOrResized (Component& component,bool wasMoved, bool wasResized) override;
    
    void setClipRefId(String id);
    String getClipRefid();
    
    void setDirty(bool dirty);
    
private:

    AudioSampleBuffer* audioBuffer;
    AudioThumbnail* thumbnail;
    AudioThumbnailCache* thumbnailCache;
    ScopedPointer<AudioFormatReaderSource> readerSource;
    ResizeConstrainer* constrainer;
    
    Rectangle<int>* thumbnailBounds = NULL;
    virtual void changeListenerCallback(ChangeBroadcaster * source) override;
    
    // reference to original file
    String clipRefId;
    
    bool dirty;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioRegion)
};


#endif  // AUDIOREGION_H_INCLUDED
