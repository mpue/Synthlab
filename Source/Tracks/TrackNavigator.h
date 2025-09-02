/*
  ==============================================================================

	WaveNavigator.h
	Created: 24 Nov 2016 8:04:54am
	Author:  Matthias Pueski

  ==============================================================================
*/

#ifndef WAVENAVIGATOR_H_INCLUDED
#define WAVENAVIGATOR_H_INCLUDED

#include <vector>
#include <map>

#include "../JuceLibraryCode/JuceHeader.h"

#include "WaveSelector.h"
#include "PositionMarker.h"
#include "Track.h"
#include "DragConstrainer.h"
#include "../MultiComponentDragger.h"
#include "../Project/TrackConfig.h"
#include "TimeLine.h"
#include "../UITools.h"
#include "../TransportDisplayComponent.h"
#include "../PianoRollComponent.h"

//==============================================================================
/*
*/
class TrackNavigator : public Component,
	public ChangeListener,
	public ChangeBroadcaster,
	public Timer,
	public KeyListener,
	public DragAndDropTarget,
	public ScrollBar::Listener,
	public EventListener

{
public:
	TrackNavigator(AudioDeviceManager* manager, Viewport* view, PropertyView* propertyView, TransportDisplayComponent* transport);
	~TrackNavigator();

	void paint(Graphics&) override;
	void resized() override;

	WaveSelector* getSelector();

	void clearTracks();

	void eventReceived(Event* e) override;

	Track* addTrack(TrackConfig* tc);
	void addTrack(Track* t);
	Track* addTrack(Track::Type type, double sampleRate);
	void removeSelectedTrack();
	Track* getCurrentTrack();
	std::vector<Track*> getTracks();
	StringArray getTrackList();

	void openPianoRoll(MidiRegion* r);
	void openEventEditor(MidiRegion* r);

	bool isPlaying();
	void setPlaying(bool playing);

	void focusGained(FocusChangeType cause) override {
		Logger::getCurrentLogger()->writeToLog("TrackNavigator: focus gained");
	}

	void focusLost(FocusChangeType cause) override {
		Logger::getCurrentLogger()->writeToLog("TrackNavigator: focus lost");
	}

	bool isRecording();
	void setRecording(bool recording);
	void noteOffForAll();

	double getMaxLength();
	void setPosition(double position);
	double getPosition();

	void setSamplePosition(int position);
	int getSamplePosition();

	void ResetMarkerPosition();

	float getZoom();
	void setZoom(float zoom);

	void updateTrackLayout(ChangeBroadcaster* source);
	void adjustHeight();

	bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;
	void itemDropped(const SourceDetails& dragSourceDetails) override;

	inline void addChangeListener(ChangeListener* listener) {
		ChangeBroadcaster::addChangeListener(listener);
	}

	void timerCallback() override;

	ValueTree getConfiguration();

	void setConfiguration(ValueTree* config);
	void scrollBarMoved(ScrollBar* scrollBarThatHasMoved, double newRangeStart) override;
		
	Region* getRecordingRegion() {
		return recordingRegion;
	}

private:

	// ComponentDragger dragger;
	// DragConstrainer constrainer;

	MultiComponentDragger* dragger = NULL;
	AudioDeviceManager* deviceManager;

	Track* currentTrack = NULL;
	PositionMarker* marker;
	std::unique_ptr<TimeLine> timeLine;
	Viewport* view;

	std::vector<Track*> tracks;
	ScopedPointer<WaveSelector> selector;

	bool playing = false;
	bool recording = false;
	double position;
	float zoom;

	int samplePosition = 0;

	// length of the track in seconds
	long tracklength;
	int mouseDeltaX = 0;

	long recordStart = 0;
	long recordStop = 0;

	int timelineOffset = 25;

	double scrollbarOffset = 0;
	
	MidiMessage message;
	PropertyView* propertyView;
	TransportDisplayComponent* transportDisplay = nullptr;

	virtual void changeListenerCallback(ChangeBroadcaster* source) override;
	virtual void mouseDown(const MouseEvent& event) override;
	virtual void mouseUp(const MouseEvent& event) override;
	virtual void mouseMove(const juce::MouseEvent& event) override;
	virtual bool keyPressed(const KeyPress& key, Component* originatingComponent) override;
	virtual void mouseDrag(const MouseEvent& event) override;

	std::unique_ptr<PianoRollComponent> pianoRoll = nullptr;

	double loopStart = 0;
	double loopEnd = 0;

	bool loopEnabled = false;

	Region* recordingRegion = nullptr;	

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackNavigator)
};


#endif  // WAVENAVIGATOR_H_INCLUDED
