// Per-track session-mode playback strategy. Holds an active region index plus
// a pending one staged by the UI; LaunchQuantizer flips pending -> active on
// a quantisation boundary via commitPending(). resolveAt() looks up the active
// region by index on the track and returns its phase-aligned local sample.
//
// Threading:
//   - setPendingRegionIndex(), getPendingRegionIndex(), getActiveRegionIndex(),
//     hasPending() are called from the UI/control thread.
//   - commitPending() is called from the audio thread on a boundary callback.
//   - resolveAt() is called from the audio thread per block.
//   The pending->active hand-off mirrors Track::CachedAutomationParam:
//   release-store the pending flag, acquire-exchange on the consumer side.
//
// Out-of-range or negative active index resolves to silence.

#pragma once

#include "IPlaybackNavigator.h"
#include <atomic>

class Track;

namespace synthlab::session {

class SessionTrackNavigator : public IPlaybackNavigator
{
public:
    explicit SessionTrackNavigator (Track* track) noexcept;

    void setPendingRegionIndex (int index) noexcept;
    int  getActiveRegionIndex  () const noexcept;
    int  getPendingRegionIndex () const noexcept;
    bool hasPending            () const noexcept;

    void commitPending () noexcept;

    PlaybackResolution resolveAt (long globalSample,
                                  long sceneStartSample) const override;

private:
    Track* track;
    std::atomic<int>  activeIndex  { -1 };
    std::atomic<int>  pendingIndex { -1 };
    std::atomic<bool> pendingFlag  { false };
};

} // namespace synthlab::session
