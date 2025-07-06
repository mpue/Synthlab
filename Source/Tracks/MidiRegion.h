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
#include "../MultiComponentDragger.h"
#include "Region.h"
#include "MidiThumbnail.h"
#include "../Project/Project.h"
#include "../ResizeConstrainer.h"

#include <map>
#include <vector>

class MidiRegion : public Region {
    
public:
    
    MidiRegion(long startSample, long sampleLength, double sampleRate, MultiComponentDragger* dragger);
    MidiRegion(MidiBuffer* buffer, double sampleRate);
    
    virtual ~MidiRegion();
    
    virtual void paint (Graphics&) override;
    virtual void resized() override;
  
    void setNumSamples(int numSamples);
    virtual int getNumSamples() override;
    
    MidiBuffer* getBuffer();
    MidiMessageSequence* getSequence();
    MidiThumbnail* getThumbnail();
    
    virtual void setThumbnailBounds(Rectangle<int>* bounds) override {
    };
    virtual Rectangle<int>* getThumbnailBounds() override {
		return nullptr;
    };

    virtual void setZoom(float zoom) override;
    virtual void setLoop(bool loop) override;
    
    // the timer is needed to updat the thumbnail during record
    virtual void timerCallback() override;    
    virtual void stopRecording() override;
    virtual void startRecording() override;
  
    void clear();
    MidiMessage* getMessage(int sampleNum);
    void addMessage(MidiMessage* m, double time, int sampleNum);
    long getNumMessages() {
        return midiMessageList.size();
    }
    MidiMessage* getMessageAt(int index) {
        return midiMessageList.at(index);
    }
    
    std::vector<MidiMessage*> getMessages() {
		return midiMessageList;
	}

    virtual void updateThumb() override;
    virtual void componentMovedOrResized (Component& component,bool wasMoved, bool wasResized) override;

    virtual void reset() override;
    
private:
    MidiBuffer* midiBuffer;

    MidiThumbnail* midiThumbnail = nullptr;
    int numSamples = 0;
    MidiBuffer::Iterator* iterator = NULL;
    MidiMessage message;
    std::map<double, MidiMessage*> midiMessages;
    std::vector<MidiMessage*> midiMessageList;
    MidiMessageSequence* sequence;
    ResizeConstrainer* constrainer = nullptr;
    int lastIndex = -1;
    long timeElapsed = 0;
    int currentMessageNum = 0;
    CriticalSection lock;

    int lastX = 0;
};


#endif  // MIDIREGION_H_INCLUDED
