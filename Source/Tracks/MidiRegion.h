/*
  ==============================================================================

    MidiRegion.h
    Created: 28 Dec 2016 6:55:16pm
    Author:  Matthias Pueski

  ==============================================================================
*/
#pragma once

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
    void rebuildThumbnail();
    
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
    vector<MidiMessage*> getMessages(int sampleNum, int tolerance);
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

    vector<MidiMessage*> getActiveMessages(int localSamplePos); 

    virtual void updateThumb() override;
    virtual void componentMovedOrResized (Component& component,bool wasMoved, bool wasResized) override;

    virtual void reset() override;
    MidiRegion* clone(long newOffset);
    std::pair<MidiRegion*, MidiRegion*> splitAt(long splitSample, MultiComponentDragger* dragger);

	void quantize(int quantizeGridSamples) override;

    MidiMessageSequence* getMidiMessageSequence();

    void setMidiMessageSequence(const MidiMessageSequence& seq);

    long getStartSample() const {
        if (midiMessageList.empty())
			return 0;
		return (long)midiMessageList.front()->getTimeStamp();
	}

    std::vector<MidiMessage*>& getMidiMessageList() { return midiMessageList; }
    const std::vector<MidiMessage*>& getMidiMessageList() const { return midiMessageList; }

    // Methode um die Region nach direkten Änderungen zu aktualisieren
    void notifyDirectEdit() {
        rebuildThumbnail();
        updateThumb();
        repaint();
    }
    std::map<double, MidiMessage*> midiMessages;
    std::vector<MidiMessage*> midiMessageList;
private:
    MidiBuffer* midiBuffer;

    MidiThumbnail* midiThumbnail = nullptr;
    int numSamples = 0;
    MidiBuffer::Iterator* iterator = NULL;
    MidiMessage message;
    MidiMessageSequence* sequence;
    ResizeConstrainer* constrainer = nullptr;
    int lastIndex = -1;
    long timeElapsed = 0;
    int currentMessageNum = 0;
    CriticalSection lock;

    int lastX = 0;

    std::map<int, MidiMessage*> activeNotes; // Note-Nummer -> aktive Note-On Message
    int lastPlaybackPosition = -1;
};