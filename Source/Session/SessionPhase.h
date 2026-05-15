// Pure phase-math for session-mode looping. Extracted so it can be tested
// without standing up Track/Region/Project. Also reused by LaunchQuantizer
// later for boundary-time calculations.

#pragma once

namespace synthlab::session::detail {

struct PhaseResult
{
    long localSample = 0;
    bool silent      = true;
};

// Returns the local sample offset within a looping region of length
// regionLength, given the global transport sample and a phase anchor.
//
// Semantics:
//   - activeIndex < 0           -> silent (no region playing)
//   - regionLength <= 0         -> silent (degenerate region length)
//   - otherwise                 -> local = (globalSample - sceneStartSample)
//                                  taken modulo regionLength, normalised to
//                                  [0, regionLength).
//
// Negative deltas (e.g. transport sample earlier than the scene-start anchor)
// wrap into the loop the same way positive ones do; this matches Ableton's
// behaviour when scrubbing backwards across a scene boundary.
inline PhaseResult computePhase (int  activeIndex,
                                 long regionLength,
                                 long globalSample,
                                 long sceneStartSample) noexcept
{
    if (activeIndex < 0 || regionLength <= 0)
        return {};

    long delta = globalSample - sceneStartSample;
    long local = delta % regionLength;
    if (local < 0) local += regionLength;
    return { local, false };
}

} // namespace synthlab::session::detail
