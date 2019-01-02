/*
  ==============================================================================

    MidiRegion.h
    Created: 28 Dec 2016 6:55:16pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#ifndef MIDIREGION_H_INCLUDED
#define MIDIREGION_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"
#include "MultiComponentDragger.h"
#include "Region.h"
#include "MidiThumbnail.h"
#include "../Project/Project.h"

#include <map>
#include <vector>

class MidiRegion : public Region {
    
public:
    
    MidiRegion(long startSample, long sampleLength, double sampleRate);
    virtual ~MidiRegion();
    
    virtual void paint (Graphics&) override;
    virtual void resized() override;

    virtual void setThumbnailBounds(Rectangle<int>* bounds) override;
    virtual Rectangle<int>* getThumbnailBounds() override;
    void setNumSamples(int numSamples);
    virtual int getNumSamples() override;
    
    MidiBuffer* getBuffer();
    MidiMessageSequence* getSequence();
    MidiThumbnail* getThumbnail();
    
    virtual void setZoom(float zoom) override;
    virtual void setLoop(bool loop) override;
    
    // the timer is needed to updat the thumbnail during record
    virtual void timerCallback() override;    
    virtual void stopRecording() override;
    virtual void startRecording() override;
    
    void clear();
    MidiMessage* getMessage(double time, int sampleNum);
    void addMessage(MidiMessage* m, double time, int sampleNum);
    
    // component dragger used to drag and drop regions
    void setDragger(MultiComponentDragger* dragger);
    
    virtual void updateThumb() override;
    virtual void componentMovedOrResized (Component& component,bool wasMoved, bool wasResized) override;

    virtual void reset() override {
        currentMessageNum = 0;
    }
    
private:
    MidiBuffer* midiBuffer;
    Rectangle<int>* thumbnailBounds = NULL;
    virtual void changeListenerCallback(ChangeBroadcaster * source) override;
    MidiThumbnail* midiThumbnail;
    int numSamples = 0;
    MidiBuffer::Iterator* iterator = NULL;
    MidiMessage message;
    std::map<double, MidiMessage*> midiMessages;
    std::vector<MidiMessage*> midiMessageList;
    MidiMessageSequence* sequence;
    int lastIndex = -1;
    long timeElapsed = 0;
    int currentMessageNum = 0;
};


#endif  // MIDIREGION_H_INCLUDED
