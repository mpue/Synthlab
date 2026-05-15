// Adapter that exposes the existing linear-timeline behaviour
// (Track::getRegionAt) through the IPlaybackNavigator interface. Lets the
// engine carry a single IPlaybackNavigator pointer per track and switch
// between arrangement and session playback without branching on a mode flag.
//
// resolveAt is in the .cpp to keep this header free of the Track/Region
// includes -- Track itself owns one of these and would otherwise pull
// in a cyclic dependency.

#pragma once

#include "IPlaybackNavigator.h"

class Track;

namespace synthlab::session {

class ArrangementPlaybackNavigator : public IPlaybackNavigator
{
public:
    explicit ArrangementPlaybackNavigator (Track* t) noexcept;

    PlaybackResolution resolveAt (long globalSample,
                                  long sceneStartSample) const override;

private:
    Track* track;
};

} // namespace synthlab::session
