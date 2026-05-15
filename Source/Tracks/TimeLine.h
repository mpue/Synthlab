/*
  ==============================================================================
    TimeLine.h
    Created: 25 Nov 2016 10:54:33am
    Author:  pueskma
    Extended: Loop functionality between two locators
  ==============================================================================
*/
#ifndef TIMELINE_H_INCLUDED
#define TIMELINE_H_INCLUDED
#include "../JuceLibraryCode/JuceHeader.h"
#include "TimeLine.h"
#include "TimelineMarker.h"
#include <vector>
//==============================================================================
/*
*/
class TimeLine : public Component, public ChangeListener
{
public:
    TimeLine(double lengthInSeconds);
    ~TimeLine();
    void paint(Graphics&) override;
    void resized() override;
    void mouseMove(const MouseEvent& event) override;
    void mouseDown(const MouseEvent& event) override;
    void mouseDrag(const MouseEvent& event) override;
    void mouseUp(const MouseEvent& event) override;

    void setLength(double length);
    static String secondsToTime(int seconds);
    void setOffset(int offset);
    void setTempoAndTimeSignature(double bpm, int numerator, int denominator);
    void setZoom(float zoom);

    // Loop-Funktionalit�t
    void setLoopEnabled(bool enabled);
    bool isLoopEnabled() const { return loopEnabled; }
    void setLoopStart(double startTime);
    void setLoopEnd(double endTime);
    double getLoopStart() const { return loopStart; }
    double getLoopEnd() const { return loopEnd; }
    void setLoopRegion(double startTime, double endTime);
    void clearLoop();

    double snapToQuarterBeat(double time) const;
    double getQuarterBeatDuration() const;
    
    // Buffer alignment functions
    double snapToBufferBoundary(double time) const;
    double getBufferDurationInSeconds() const;
    int getCurrentBufferSize() const;
    double getCurrentSampleRate() const;
    double getMinLoopDuration() const;

    // Callback f�r Loop-�nderungen
    std::function<void(double, double, bool)> onLoopChanged;

    // ===== Wavelab-style Marker =====
    // Marker werden hier verwaltet, weil die TimeLine das einzige Component ist,
    // das schon Pixel<->Zeit-Mapping plus Maus-Interaktion fuer Locatoren hat —
    // doppelt waere das gleiche Setup im TrackNavigator. TrackNavigator liest
    // die Liste fuer die Loop-Playback-Logik, persistiert sie in der Tracks-XML
    // und triggert resetLoopCounters() beim Play-Start.
    const std::vector<TimelineMarker>& getMarkers() const { return markers; }
    std::vector<TimelineMarker>&       getMarkersMutable() { return markers; }

    void addMarker(const TimelineMarker& m);
    void removeMarker(int index);
    void clearMarkers();
    void resetLoopCounters();

    // Wird nach jeder Mutation (Add/Move/Edit/Delete) gefeuert — TrackNavigator
    // haengt sich dran um Dirty-Flags zu setzen.
    std::function<void()> onMarkersChanged;

private:
    void showMarkerContextMenu(int markerIndex, juce::Point<int> screenPos);
    void showEmptyContextMenu(double timeAtClick, juce::Point<int> screenPos);
    int  markerIndexAt(const juce::MouseEvent& event, float tolerance = 6.0f) const;
    void renameMarker(int index);
    void setLoopRepeats(int index);
    void notifyChanged();

    virtual void changeListenerCallback(ChangeBroadcaster* source) override;
    double pixelToTime(float pixel) const;
    float timeToPixel(double time) const;
    bool isMouseOverLocator(const MouseEvent& event, double locatorTime, float tolerance = 5.0f) const;

    double length;
    int offset = 0;
    std::unique_ptr<DropShadow> dropShadow;
    std::unique_ptr<DropShadower> dropShadower;
    double bpm = 120.0;
    int timeSigNumerator = 4;
    int timeSigDenominator = 4;
    float zoom = 40.0f;

    // Loop-Parameter
    bool loopEnabled = false;
    double loopStart = 0.0;
    double loopEnd = 0.0;

    // Marker-Liste. Sortiert nach position — addMarker/Drag halten die
    // Sortierung aufrecht, damit "vorheriger Marker" trivial per linearer Suche
    // gefunden wird (Listen werden in der Praxis nie gross genug fuer mehr).
    std::vector<TimelineMarker> markers;
    int draggingMarker = -1;       // Index waehrend eines Drags (oder -1)

    // Mouse-Interaktion
    enum DragMode {
        None,
        DragLoopStart,
        DragLoopEnd,
        DragLoopRegion,
        DragMarker
    };

    DragMode currentDragMode = None;
    double dragStartOffset = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimeLine)
};
#endif  // TIMELINE_H_INCLUDED