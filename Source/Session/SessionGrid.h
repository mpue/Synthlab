// Tracks x scenes view over existing Track/Region state. Pure UI-thread
// facade: a slot cell (track t, scene s) resolves to the s-th Region on
// Track t, or Empty if the track has fewer regions. Launch actions
// translate clicks into LaunchQuantizer schedules; the audio thread does
// not touch SessionGrid.
//
// SessionGrid does not own its tracks or navigators. Caller supplies them
// as {Track*, SessionTrackNavigator*} columns; navigator lifetime is
// expected to outlive the grid. Re-build the grid on topology changes
// (track add/remove); this version does not observe Track lifetimes.

#pragma once

#include "LaunchQuantizer.h"
#include <vector>

class Track;
class Region;

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

class SessionGrid
{
public:
    struct Column
    {
        Track*                 track     = nullptr;
        SessionTrackNavigator* navigator = nullptr;
    };

    SessionGrid (std::vector<Column> columns, LaunchQuantizer* quantizer);

    int getNumTracks() const noexcept;
    int getNumScenes() const noexcept;  // max(numRegions) across tracks

    Track*                 getTrack     (int trackIndex) const;
    SessionTrackNavigator* getNavigator (int trackIndex) const;

    Region*   getRegionAt  (int trackIndex, int sceneIndex) const;
    SlotState getSlotState (int trackIndex, int sceneIndex) const;

    // Single click on a slot. Empty cell -> stop (newRegionIndex = -1).
    void launchSlot  (int trackIndex, int sceneIndex,
                      Quantization q, long currentTransportSample);

    // Click on a scene row header. Atomic across all tracks: tracks with
    // no region at sceneIndex receive a stop schedule on the same boundary.
    void launchScene (int sceneIndex,
                      Quantization q, long currentTransportSample);

    void stopTrack (int trackIndex,
                    Quantization q, long currentTransportSample);
    void stopAll   (Quantization q, long currentTransportSample);

private:
    std::vector<Column> columns;
    LaunchQuantizer*    quantizer = nullptr;
};

} // namespace synthlab::session
