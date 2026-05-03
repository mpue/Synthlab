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
#include "../MessageBus/FastBus.h"

#include <map>
#include <vector>

class MidiRegion : public Region {
    
public:
    
    MidiRegion(long startSample, long sampleLength, double sampleRate, MultiComponentDragger* dragger);
    MidiRegion(MidiBuffer* buffer, double sampleRate);
    
    virtual ~MidiRegion();
    
    virtual void paint (Graphics&) override;
    virtual void resized() override;

    void mouseDoubleClick(const MouseEvent& e) override;
  
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
    
    // the timer is needed to update the thumbnail during record
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

    // Setzt den MIDI-Channel aller gespeicherten Messages (note on/off, CC, …)
    // auf den angegebenen Wert (1..16). Notwendig nach einem Cross-Track-Paste:
    // TrackIn::eventReceived verwirft Events, deren Channel nicht zum neuen
    // Track passt — sonst wäre die kopierte Region stumm.
    void setMidiChannelForAllMessages(int channel);
    std::pair<MidiRegion*, MidiRegion*> splitAt(long splitSample, MultiComponentDragger* dragger);

	void quantize(int quantizeGridSamples) override;
    void consolidate() override;

    MidiMessageSequence getMidiMessageSequence();

    void setMidiMessageSequence(const MidiMessageSequence& seq);

    long getStartSample() const {
        if (midiMessageList.empty())
			return 0;
		return (long)midiMessageList.front()->getTimeStamp();
	}

    std::vector<MidiMessage*>& getMidiMessageList() { return midiMessageList; }
    const std::vector<MidiMessage*>& getMidiMessageList() const { return midiMessageList; }

    void sanitize();

    // Methode um die Region nach direkten �nderungen zu aktualisieren
    void notifyDirectEdit() {
        rebuildThumbnail();
        updateThumb();
        repaint();
    }
    std::map<double, MidiMessage*> midiMessages;
    std::vector<MidiMessage*> midiMessageList;
private:

    // Tempo-bezogene Methoden
    void updateTempoFromProject();
    void updateBoundsFromTempo();
    void rescaleMidiEvents(double oldBPM, double newBPM);
    void handleTempoChange(double newBPM);
    void setTempo(double bpm);
    double getCurrentBPM() const;
    long beatsToSamples(double beats) const;
    double samplesToBeats(long samples) const;
    void setQuantizeGrid(double beats);
    double getQuantizeGrid() const;
    void setSnapToGrid(bool snap);
    bool getSnapToGrid() const;
    void quantizeToBeats(double beatGrid);
    long snapSampleToGrid(long sample) const;
    MidiBuffer* midiBuffer;
    int calculateActualLength() const;
    MidiThumbnail* midiThumbnail = nullptr;
    int numSamples = 0;
    // True waehrend consolidate() — verhindert, dass das interne setBounds
    // im resize-Handler die rechte Kante hochsnappt und numSamples wachsen
    // laesst (das wuerde nach Save/Reload den Nachbarn ueberlappen).
    bool consolidateInProgress = false;

    // Sample-Offset INNERHALB des Message-Buffers, an dem die linke Kante der
    // Region beginnt. Wird beim Links-Trim erhoeht, damit die Notenpositionen
    // im Timeline-System unveraendert bleiben (Noten links der neuen linken
    // Kante werden geclippt; Noten rechts bleiben am gleichen Platz).
    long sourceStartSample = 0;

public:
    long getSourceStartSample() const override { return sourceStartSample; }
    void setSourceStartSample(long s) { sourceStartSample = juce::jmax(0L, s); }
private:
    std::shared_ptr<bool> tempoSubscriptionAlive;
    MidiBuffer::Iterator* iterator = NULL;
    MidiMessage message;
    MidiMessageSequence* sequence;
    ResizeConstrainer* constrainer = nullptr;
    int lastIndex = -1;
    long timeElapsed = 0;
    int currentMessageNum = 0;
    CriticalSection lock;
	bool snapToGrid = true;
    int lastX = 0;
    double currentBPM;
    double quantizeGridBeats;
    std::map<int, MidiMessage*> activeNotes; // Note-Nummer -> aktive Note-On Message
    int lastPlaybackPosition = -1;
};