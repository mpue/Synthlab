// Schedules region-launch commits for SessionTrackNavigators on musical
// boundaries derived from a global transport. The UI thread enqueues
// launches via schedule()/scheduleScene(); the audio thread polls per block
// via takeBlockEvents() and applies commitPending() at the right sample
// offset so the swap is sample-accurate inside the buffer.
//
// Scene atomicity falls out of the queue model: every entry in a scene
// launch shares the same commitAtSample, so all navigators flip on the
// same sample without a separate "group" concept.

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include <vector>

namespace synthlab::session {

class SessionTrackNavigator;

enum class Quantization
{
    None,            // commit at the earliest opportunity (this block or next)
    OneSixteenth,
    OneEighth,
    OneQuarter,
    OneBar,
    TwoBars,
    FourBars,
    EightBars
    // NextScene deliberately absent: requires scene-length wiring that will
    // come with SessionGrid.
};

struct LaunchCommitEvent
{
    int                    sampleOffsetInBlock = 0; // 0 <= offset < blockSize
    SessionTrackNavigator* navigator           = nullptr;
};

class LaunchQuantizer
{
public:
    LaunchQuantizer() = default;

    // UI thread. tempoBpm refers to quarter notes regardless of time
    // signature -- a bar in 6/8 at 120 bpm is 3 quarter-note durations.
    void setTransport (double sampleRate,
                       double tempoBpm,
                       int    timeSigNumerator   = 4,
                       int    timeSigDenominator = 4);

    // UI thread. Plugin delay compensation: launches are quantised against
    // (currentTransportSample + pdcOffset) so the audible result lands on
    // the musical grid even when the audio engine is running ahead of the
    // user-visible playhead.
    void setPdcOffsetSamples (long samples);

    // UI thread. Schedule a single launch. newRegionIndex = -1 stops the
    // track. The navigator's pending region index is set immediately; the
    // queue records when commitPending() should be called. Any prior
    // pending launch for the same navigator is overwritten -- only the
    // most recent intent fires.
    void schedule (SessionTrackNavigator* nav,
                   int                    newRegionIndex,
                   Quantization           q,
                   long                   currentTransportSample);

    struct SceneEntry
    {
        SessionTrackNavigator* nav            = nullptr;
        int                    newRegionIndex = -1;
    };

    // UI thread. Schedule a whole scene -- one commit boundary, every entry
    // flips on the same sample.
    void scheduleScene (const SceneEntry* entries,
                        int               count,
                        Quantization      q,
                        long              currentTransportSample);

    // Audio thread. Appends commit events for any scheduled launches whose
    // commitAtSample falls inside [blockStartSample, blockStartSample + blockSize).
    // Events are emitted in ascending sampleOffsetInBlock order. A late
    // entry (commitAtSample < blockStartSample) is emitted at offset 0.
    // Caller invokes navigator->commitPending() at each emitted offset.
    //
    // `out` is cleared on entry; pass the same vector across blocks to
    // avoid reallocation.
    void takeBlockEvents (long                            blockStartSample,
                          int                             blockSize,
                          std::vector<LaunchCommitEvent>& out);

    int getPendingCount() const;

private:
    struct Scheduled
    {
        SessionTrackNavigator* nav            = nullptr;
        long                   commitAtSample = 0;
    };

    long quantisationStrideSamples (Quantization q) const;
    long computeCommitAtSample     (long currentSample, Quantization q) const;
    void replaceOrAppend           (SessionTrackNavigator* nav, long commitAtSample);

    // UI thread schedule() and audio thread takeBlockEvents() share this
    // lock. Critical sections are short (queue push / scan) so the audio
    // thread blocking risk is acceptable, mirroring PluginModule's lock
    // usage. If profiling shows contention, migrate to juce::AbstractFifo.
    mutable juce::CriticalSection lock;

    std::vector<Scheduled> scheduled;

    double sampleRate = 44100.0;
    double tempoBpm   = 120.0;
    int    timeSigNum = 4;
    int    timeSigDen = 4;
    long   pdcOffset  = 0;
};

} // namespace synthlab::session
