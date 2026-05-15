#include "LaunchQuantizer.h"
#include "SessionTrackNavigator.h"
#include <algorithm>

namespace synthlab::session {

void LaunchQuantizer::setTransport (double sr, double bpm, int num, int den)
{
    const juce::ScopedLock l (lock);
    sampleRate = sr;
    tempoBpm   = bpm;
    timeSigNum = num;
    timeSigDen = den;
}

void LaunchQuantizer::setPdcOffsetSamples (long samples)
{
    const juce::ScopedLock l (lock);
    pdcOffset = samples;
}

void LaunchQuantizer::schedule (SessionTrackNavigator* nav,
                                int                    newRegionIndex,
                                Quantization           q,
                                long                   currentTransportSample)
{
    if (nav == nullptr) return;

    // Stage the index on the navigator straight away -- atomic, no lock
    // needed. The queue records only "when to flip", not what to flip to.
    nav->setPendingRegionIndex (newRegionIndex);

    const juce::ScopedLock l (lock);
    const long commitAt = computeCommitAtSample (currentTransportSample, q);
    replaceOrAppend (nav, commitAt);
}

void LaunchQuantizer::scheduleScene (const SceneEntry* entries,
                                     int               count,
                                     Quantization      q,
                                     long              currentTransportSample)
{
    if (entries == nullptr || count <= 0) return;

    const juce::ScopedLock l (lock);
    const long commitAt = computeCommitAtSample (currentTransportSample, q);
    for (int i = 0; i < count; ++i)
    {
        auto* nav = entries[i].nav;
        if (nav == nullptr) continue;
        nav->setPendingRegionIndex (entries[i].newRegionIndex);
        replaceOrAppend (nav, commitAt);
    }
}

void LaunchQuantizer::takeBlockEvents (long                            blockStartSample,
                                       int                             blockSize,
                                       std::vector<LaunchCommitEvent>& out)
{
    out.clear();
    if (blockSize <= 0) return;

    const long blockEnd = blockStartSample + (long) blockSize;

    const juce::ScopedLock l (lock);

    for (auto it = scheduled.begin(); it != scheduled.end(); )
    {
        if (it->commitAtSample < blockEnd)
        {
            long offset = it->commitAtSample - blockStartSample;
            if (offset < 0) offset = 0; // late entry: fire immediately
            if (offset >= blockSize) offset = blockSize - 1;

            LaunchCommitEvent ev;
            ev.sampleOffsetInBlock = (int) offset;
            ev.navigator           = it->nav;
            out.push_back (ev);

            it = scheduled.erase (it);
        }
        else
        {
            ++it;
        }
    }

    std::sort (out.begin(), out.end(),
               [] (const LaunchCommitEvent& a, const LaunchCommitEvent& b)
               { return a.sampleOffsetInBlock < b.sampleOffsetInBlock; });
}

int LaunchQuantizer::getPendingCount() const
{
    const juce::ScopedLock l (lock);
    return (int) scheduled.size();
}

long LaunchQuantizer::quantisationStrideSamples (Quantization q) const
{
    if (q == Quantization::None || tempoBpm <= 0.0 || sampleRate <= 0.0)
        return 0;

    const double samplesPerQuarter = sampleRate * 60.0 / tempoBpm;
    const double barSamples = samplesPerQuarter
                            * (double) timeSigNum * 4.0 / (double) timeSigDen;

    switch (q)
    {
        case Quantization::OneSixteenth: return (long) (samplesPerQuarter * 0.25);
        case Quantization::OneEighth:    return (long) (samplesPerQuarter * 0.5);
        case Quantization::OneQuarter:   return (long)  samplesPerQuarter;
        case Quantization::OneBar:       return (long)  barSamples;
        case Quantization::TwoBars:      return (long) (barSamples * 2.0);
        case Quantization::FourBars:     return (long) (barSamples * 4.0);
        case Quantization::EightBars:    return (long) (barSamples * 8.0);
        case Quantization::None:         return 0;
    }
    return 0;
}

long LaunchQuantizer::computeCommitAtSample (long currentSample, Quantization q) const
{
    const long stride   = quantisationStrideSamples (q);
    const long adjusted = currentSample + pdcOffset;
    if (stride <= 0)
        return adjusted;

    long rem = adjusted % stride;
    if (rem < 0) rem += stride;
    if (rem == 0) return adjusted;
    return adjusted + (stride - rem);
}

void LaunchQuantizer::replaceOrAppend (SessionTrackNavigator* nav, long commitAtSample)
{
    auto it = std::find_if (scheduled.begin(), scheduled.end(),
                            [nav] (const Scheduled& s) { return s.nav == nav; });
    if (it != scheduled.end())
        it->commitAtSample = commitAtSample;
    else
        scheduled.push_back ({ nav, commitAtSample });
}

} // namespace synthlab::session
