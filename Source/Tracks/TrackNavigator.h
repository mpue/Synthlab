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
#include "TrackControl.h"

class SynthEditor;
//==============================================================================

// Big centered zoom-level overlay. Fades in on zoom changes and fades back
// out about a second after the last change. Lives inside TrackNavigator and
// is repositioned to stay centered in the visible viewport area.
class ZoomDisplay : public juce::Component, private juce::Timer
{
public:
	ZoomDisplay()
	{
		setInterceptsMouseClicks(false, false);
		setAlwaysOnTop(true);
		setAlpha(0.0f);
		setVisible(false);
	}

	void showZoom(float zoomLevel)
	{
		currentZoom = zoomLevel;
		// Hold full opacity until 1 s after the most recent zoom event.
		// Successive showZoom calls during a fast +/- burst keep extending
		// the hold so the display doesn't flicker between fade-in/fade-out.
		holdUntilMs = juce::Time::getMillisecondCounterHiRes() + 1000.0;
		fadingOut = false;
		if (!isVisible())
			setVisible(true);
		toFront(false);
		if (!isTimerRunning())
			startTimerHz(60);
		repaint();
	}

	void paint(juce::Graphics& g) override
	{
		auto bounds = getLocalBounds().toFloat();
		g.setColour(juce::Colours::black.withAlpha(0.65f));
		g.fillRoundedRectangle(bounds, 18.0f);
		g.setColour(juce::Colours::white.withAlpha(0.25f));
		g.drawRoundedRectangle(bounds.reduced(1.0f), 18.0f, 1.5f);

		g.setColour(juce::Colours::white);
		g.setFont(juce::Font(40.0f, juce::Font::bold));
		// Allow drawFittedText to shrink horizontally down to 50% so longer
		// values (e.g. four-digit zoom) don't get truncated to "...".
		g.drawFittedText("Zoom: " + juce::String(currentZoom, 1),
		                 getLocalBounds().reduced(20),
		                 juce::Justification::centred, 1, 0.5f);
	}

private:
	void timerCallback() override
	{
		const double now = juce::Time::getMillisecondCounterHiRes();
		float a = getAlpha();

		if (now < holdUntilMs)
		{
			// Fade in (or stay at full opacity once we're there).
			fadingOut = false;
			if (a < 1.0f)
				a = std::min(1.0f, a + 0.18f);
		}
		else
		{
			// Hold expired — fade out.
			fadingOut = true;
			a -= 0.06f;
			if (a <= 0.0f)
			{
				a = 0.0f;
				setAlpha(0.0f);
				setVisible(false);
				stopTimer();
				return;
			}
		}
		setAlpha(a);
	}

	float currentZoom = 0.0f;
	double holdUntilMs = 0.0;
	bool fadingOut = false;
};

/*
*/
class TrackNavigator : public Component,
	public ChangeListener,
	public ChangeBroadcaster,
	public Timer,
	public KeyListener,
	public DragAndDropTarget,
	public FileDragAndDropTarget,
	public ScrollBar::Listener,
	public EventListener

{
public:
	TrackNavigator(AudioDeviceManager* manager, Viewport* view, PropertyView* propertyView, TransportDisplayComponent* transport);
	~TrackNavigator();

	void paint(Graphics&) override;
	void resized() override;
	// Wird vom Viewport aufgerufen, wenn der Navigator verschoben wird (z.B.
	// beim horizontalen Scrollen). Wir nutzen das, um die Track-Control-Strips
	// am sichtbaren linken Rand zu fixieren.
	void moved() override;
	// Sorgt dafür, dass der Navigator auch ohne Tracks die volle sichtbare
	// Viewport-Fläche abdeckt — sonst landen Drops auf "leerem" Bereich nicht
	// im Navigator und es entsteht kein neuer Audio-Track.
	void parentSizeChanged() override;
	void parentHierarchyChanged() override;

	WaveSelector* getSelector();

	void clearTracks();

	void eventReceived(Event* e) override;

	Track* addTrack(TrackConfig* tc);
	void addTrack(Track* t);
	Track* addTrack(Track::Type type, double sampleRate);

	// Liefert den Track, dessen vertikaler Bereich Y enthält, oder nullptr,
	// wenn an dieser Y-Position kein Track liegt (z.B. unter dem letzten
	// Track oder oberhalb der Tracks im Timeline-Bereich).
	Track* trackAtY(int y) const;

	// Optionaler Rückverweis auf den SynthEditor — wird benötigt, damit
	// per Drag&Drop angelegte Audio-Tracks dieselbe Verdrahtung
	// (AudioIn L/R, TrackIn, AudioOut) bekommen wie über den Menü-Pfad.
	void setEditor(SynthEditor* e) { editor = e; }
	SynthEditor* getEditor() const { return editor; }
	void removeSelectedTrack();
	Track* detachSelectedTrack(int& outIndex);
	void insertTrackAt(Track* track, int index);
	Track* getCurrentTrack();
	std::vector<Track*> getTracks();
	StringArray getTrackList();

	// Stellt sicher, dass jeder Track einen TrackControl-Strip links daneben
	// hat (an x=0, gleiche Y wie der Track). Erzeugt fehlende, entfernt
	// verwaiste, positioniert alle. Idempotent — kann nach jeder Track-
	// Mutation aufgerufen werden.
	void syncTrackControls();

	void openPianoRoll(MidiRegion* r);
	void openEventEditor(MidiRegion* r);

	bool isPlaying();
	void setPlaying(bool playing);

	void focusGained(FocusChangeType cause) override {
		hasFocus = true;
	}

	void focusLost(FocusChangeType cause) override {
		hasFocus = false;		
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

	int alignToBufferSize(int samplePosition);

	void updateTrackLayout(ChangeBroadcaster* source);
	void adjustHeight();

	bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;
	void itemDropped(const SourceDetails& dragSourceDetails) override;

	bool isInterestedInFileDrag(const StringArray& files) override;
	void filesDropped(const StringArray& files, int x, int y) override;

	inline void addChangeListener(ChangeListener* listener) {
		ChangeBroadcaster::addChangeListener(listener);
	}

	void allNotesOff();

	void timerCallback() override;

	ValueTree getConfiguration();

	void setConfiguration(ValueTree* config);
	void scrollBarMoved(ScrollBar* scrollBarThatHasMoved, double newRangeStart) override;
	void mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) override;
		
	Region* getRecordingRegion() {
		return recordingRegion;
	}

private:
	// Legt für einen Drop einen Audio-Track an. Geht möglichst über
	// TrackUtil::createAudioTrack, damit dieselbe Verdrahtung wie beim
	// Menü-Pfad entsteht; fällt sonst auf addTrack(Type::AUDIO,…) zurück.
	Track* createAudioTrackForDrop();

	MultiComponentDragger* dragger = NULL;
	AudioDeviceManager* deviceManager;

	Track* currentTrack = NULL;
	PositionMarker* marker;
	std::unique_ptr<TimeLine> timeLine;
	Viewport* view;

	std::vector<Track*> tracks;
	// 1:1 zu `tracks` — Index N gehört zum Track N. TrackControls liegen
	// links bei x=0..kControlsWidth, gleiche Y wie ihr Track.
	std::vector<std::unique_ptr<TrackControl>> trackControls;
	std::unique_ptr<WaveSelector> selector;

public:
	// Breite des linken Steuer-Strips, ab dem die eigentlichen Tracks/Marker/
	// Timeline beginnen. Region-Coords im Track-internen System sind davon
	// nicht betroffen — nur das Layout im Navigator.
	static constexpr int kControlsWidth = 140;
private:

	std::atomic<bool> playing { false };
	std::atomic<bool> recording { false };
	double position;
	float zoom;

	std::atomic<int> samplePosition { 0 };

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

	bool hasFocus = false;

	bool loopEnabled = false;

	Region* recordingRegion = nullptr;

	std::unique_ptr<ZoomDisplay> zoomDisplay;

	SynthEditor* editor = nullptr;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackNavigator)
};


#endif  // WAVENAVIGATOR_H_INCLUDED
