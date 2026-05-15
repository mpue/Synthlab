#include "SessionTrackNavigator.h"
#include "SessionPhase.h"
#include "../Tracks/Region.h"
#include "../Tracks/Track.h"

namespace synthlab::session {

SessionTrackNavigator::SessionTrackNavigator (Track* t) noexcept
    : track (t) {}

void SessionTrackNavigator::setPendingRegionIndex (int index) noexcept
{
    pendingIndex.store (index, std::memory_order_relaxed);
    pendingFlag .store (true,  std::memory_order_release);
}

int SessionTrackNavigator::getActiveRegionIndex () const noexcept
{
    return activeIndex.load (std::memory_order_acquire);
}

int SessionTrackNavigator::getPendingRegionIndex () const noexcept
{
    return pendingIndex.load (std::memory_order_relaxed);
}

bool SessionTrackNavigator::hasPending () const noexcept
{
    return pendingFlag.load (std::memory_order_acquire);
}

void SessionTrackNavigator::commitPending () noexcept
{
    if (! pendingFlag.exchange (false, std::memory_order_acquire))
        return;

    const int idx = pendingIndex.load (std::memory_order_relaxed);
    activeIndex.store (idx, std::memory_order_release);
}

PlaybackResolution SessionTrackNavigator::resolveAt (long globalSample,
                                                     long sceneStartSample) const
{
    if (track == nullptr)
        return {};

    const int idx = activeIndex.load (std::memory_order_acquire);
    if (idx < 0)
        return {};

    auto regions = track->getRegions();
    if (idx >= (int) regions.size())
        return {};

    Region* r = regions[(size_t) idx];
    if (r == nullptr)
        return {};

    const long len = (long) r->getNumSamples();
    auto phase = detail::computePhase (idx, len, globalSample, sceneStartSample);
    if (phase.silent)
        return {};

    return { r, phase.localSample, false };
}

} // namespace synthlab::session
