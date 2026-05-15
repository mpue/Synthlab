// Abstraction over "which Region plays at transport position T, and where in
// that region's content are we?" Two intended implementations:
//   - ArrangementPlaybackNavigator: linear timeline lookup (wraps the
//     existing Track::getRegionAt).
//   - SessionTrackNavigator: holds an active region per track and loops it
//     phase-aligned to a scene-start anchor.
//
// resolveAt() is called on the audio thread; implementations must not allocate
// or take locks on that path.

#pragma once

class Region;

namespace synthlab::session {

struct PlaybackResolution
{
    Region* region      = nullptr; // nullptr <=> silent
    long    localSample = 0;       // sample offset into the region's content
    bool    silent      = true;
};

class IPlaybackNavigator
{
public:
    virtual ~IPlaybackNavigator() = default;

    // globalSample      : transport sample, 0 == song start.
    // sceneStartSample  : phase anchor (sample at which the active region
    //                     started). Arrangement implementations ignore it;
    //                     session implementations use it for loop phase.
    virtual PlaybackResolution resolveAt (long globalSample,
                                          long sceneStartSample) const = 0;
};

} // namespace synthlab::session
