// Adapter that exposes the existing linear-timeline behaviour
// (Track::getRegionAt) through the IPlaybackNavigator interface. Lets the
// engine carry a single IPlaybackNavigator pointer per track and switch
// between arrangement and session playback without branching on a mode flag.

#pragma once

#include "IPlaybackNavigator.h"
#include "../Tracks/Region.h"
#include "../Tracks/Track.h"

namespace synthlab::session {

class ArrangementPlaybackNavigator : public IPlaybackNavigator
{
public:
    explicit ArrangementPlaybackNavigator (Track* t) : track (t) {}

    PlaybackResolution resolveAt (long globalSample,
                                  long /*sceneStartSample*/) const override
    {
        if (track == nullptr)
            return {};

        Region* r = track->getRegionAt (globalSample);
        if (r == nullptr)
            return {};

        return { r, globalSample - r->getSampleOffset(), false };
    }

private:
    Track* track;
};

} // namespace synthlab::session
