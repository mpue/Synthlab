#include "ArrangementPlaybackNavigator.h"
#include "../Tracks/Region.h"
#include "../Tracks/Track.h"

namespace synthlab::session {

ArrangementPlaybackNavigator::ArrangementPlaybackNavigator (Track* t) noexcept
    : track (t) {}

PlaybackResolution ArrangementPlaybackNavigator::resolveAt (long globalSample,
                                                            long /*sceneStartSample*/) const
{
    if (track == nullptr)
        return {};

    Region* r = track->getRegionAt (globalSample);
    if (r == nullptr)
        return {};

    return { r, globalSample - r->getSampleOffset(), false };
}

} // namespace synthlab::session
