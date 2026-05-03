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
#include "../MultiComponentDragger.h"
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
       
    float getPitch();
    void setPitch(float pitch);

    virtual juce::Array<juce::PropertyComponent*>& getProperties() override;
    virtual void createProperties() override;

    void resizeAudio(int length);

    void quantize(int quantizeGridSamples) override {
		// empty implementation, audio regions are not quantized
    }

    void consolidate() override;

    // Sample-Offset INNERHALB des Audio-Buffers, an dem die linke Kante der
    // Region beginnt. Wird beim Links-Trim erhoeht; getNumSamples() bleibt
    // die sichtbare (bzw. spielende) Laenge ab diesem Offset. consolidate()
    // baked das in den Buffer ein und setzt zurueck auf 0.
    long getSourceStartSample() const override { return sourceStartSample; }
    void setSourceStartSample(long s) { sourceStartSample = juce::jmax(0L, s); }

private:

    struct PitchListener : juce::Value::Listener
    {
        PitchListener(juce::Value& v, AudioRegion* r) : region(r), value(v) { value.addListener(this); }
        ~PitchListener() {}  // no need to remove the listener

        void valueChanged(juce::Value& value) override {
            region->setPitch(value.toString().getFloatValue());
        }
        AudioRegion* region;
        juce::Value value;
    };


    AudioSampleBuffer* audioBuffer;
    AudioSampleBuffer* audioBufferProcessed = nullptr;
    long sourceStartSample = 0;
    AudioThumbnail* thumbnail;
    AudioThumbnailCache* thumbnailCache;
    ScopedPointer<AudioFormatReaderSource> readerSource;
    ResizeConstrainer* constrainer;

    double getQuarterNoteLength();
    long snapToQuarterNote(long samplePosition);
    void updateSnapSettings();

    Rectangle<int>* thumbnailBounds = NULL;
    virtual void changeListenerCallback(ChangeBroadcaster * source) override;
    
    // reference to original file
    String clipRefId;
    float pitch = 1.0f;

    PitchListener* pitchListener;
    juce::Value* pitchValue;
    juce::PropertyComponent* pitchProp;

    float* tempBufferLeft = nullptr;
    float* tempBufferRight = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioRegion)
};


#endif  // AUDIOREGION_H_INCLUDED
