// Tracks x scenes view over existing Track/Region state. Pure UI-thread
// facade: a slot cell (track t, scene s) resolves to the s-th Region on
// Track t, or Empty if the track has fewer regions. Launch actions
// translate clicks into LaunchQuantizer schedules; the audio thread does
// not touch SessionGrid.
//
// The grid listens to the TrackNavigator and rebuilds its column snapshot
// whenever tracks are added, removed, renamed, etc., so the open Session
// View stays in sync with the arrangement.

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "LaunchQuantizer.h"
#include <vector>

class Region;
class Track;
class TrackNavigator;

namespace synthlab::session {

class SessionTrackNavigator;

enum class SlotState
{
    Empty,      // no region at this (track, scene) cell
    HasRegion,  // region present, not playing, not queued
    Queued,     // navigator has a pending launch pointing at this scene
    Playing,    // navigator's active region index == sceneIndex
    Recording   // reserved: not emitted until recording-target flag exists
};

class SessionGrid : private juce::ChangeListener
{
public:
    SessionGrid (TrackNavigator* tracks, LaunchQuantizer* quantizer);
    ~SessionGrid() override;

    int getNumTracks() const noexcept;
    int getNumScenes() const noexcept;  // max(numRegions) across tracks

    Track*                 getTrack     (int trackIndex) const;
    SessionTrackNavigator* getNavigator (int trackIndex) const;

    Region*   getRegionAt  (int trackIndex, int sceneIndex) const;
    SlotState getSlotState (int trackIndex, int sceneIndex) const;

    void launchSlot  (int trackIndex, int sceneIndex,
                      Quantization q, long currentTransportSample);
    void launchScene (int sceneIndex,
                      Quantization q, long currentTransportSample);
    void stopTrack (int trackIndex,
                    Quantization q, long currentTransportSample);
    void stopAll   (Quantization q, long currentTransportSample);

private:
    struct Column
    {
        Track*                 track     = nullptr;
        SessionTrackNavigator* navigator = nullptr;
    };

    void rebuild();
    void changeListenerCallback (juce::ChangeBroadcaster* source) override;

    TrackNavigator*     trackNav  = nullptr;
    LaunchQuantizer*    quantizer = nullptr;
    std::vector<Column> columns;
};

} // namespace synthlab::session
