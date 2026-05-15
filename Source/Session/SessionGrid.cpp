#include "SessionGrid.h"
#include "SessionTrackNavigator.h"
#include "../Tracks/Region.h"
#include "../Tracks/Track.h"

namespace synthlab::session {

SessionGrid::SessionGrid (std::vector<Column> cols, LaunchQuantizer* q)
    : columns (std::move (cols)), quantizer (q) {}

int SessionGrid::getNumTracks() const noexcept
{
    return (int) columns.size();
}

int SessionGrid::getNumScenes() const noexcept
{
    int maxRegions = 0;
    for (const auto& c : columns)
    {
        if (c.track == nullptr) continue;
        const int n = c.track->getNumRegions();
        if (n > maxRegions) maxRegions = n;
    }
    return maxRegions;
}

Track* SessionGrid::getTrack (int i) const
{
    if (i < 0 || i >= (int) columns.size()) return nullptr;
    return columns[(size_t) i].track;
}

SessionTrackNavigator* SessionGrid::getNavigator (int i) const
{
    if (i < 0 || i >= (int) columns.size()) return nullptr;
    return columns[(size_t) i].navigator;
}

Region* SessionGrid::getRegionAt (int trackIndex, int sceneIndex) const
{
    Track* t = getTrack (trackIndex);
    if (t == nullptr) return nullptr;
    return t->getRegionByIndex (sceneIndex);
}

SlotState SessionGrid::getSlotState (int trackIndex, int sceneIndex) const
{
    if (getRegionAt (trackIndex, sceneIndex) == nullptr)
        return SlotState::Empty;

    SessionTrackNavigator* nav = getNavigator (trackIndex);
    if (nav == nullptr)
        return SlotState::HasRegion;

    if (nav->getActiveRegionIndex() == sceneIndex)
        return SlotState::Playing;

    if (nav->hasPending() && nav->getPendingRegionIndex() == sceneIndex)
        return SlotState::Queued;

    return SlotState::HasRegion;
}

void SessionGrid::launchSlot (int  trackIndex,
                              int  sceneIndex,
                              Quantization q,
                              long currentTransportSample)
{
    if (quantizer == nullptr) return;

    SessionTrackNavigator* nav = getNavigator (trackIndex);
    if (nav == nullptr) return;

    const int newIndex = (getRegionAt (trackIndex, sceneIndex) != nullptr)
                       ? sceneIndex : -1;
    quantizer->schedule (nav, newIndex, q, currentTransportSample);
}

void SessionGrid::launchScene (int  sceneIndex,
                               Quantization q,
                               long currentTransportSample)
{
    if (quantizer == nullptr) return;

    std::vector<LaunchQuantizer::SceneEntry> entries;
    entries.reserve (columns.size());

    for (const auto& c : columns)
    {
        if (c.navigator == nullptr) continue;

        int newIndex = -1;
        if (c.track != nullptr
            && c.track->getRegionByIndex (sceneIndex) != nullptr)
            newIndex = sceneIndex;
        entries.push_back ({ c.navigator, newIndex });
    }

    if (entries.empty()) return;
    quantizer->scheduleScene (entries.data(), (int) entries.size(),
                              q, currentTransportSample);
}

void SessionGrid::stopTrack (int trackIndex,
                             Quantization q,
                             long currentTransportSample)
{
    if (quantizer == nullptr) return;
    SessionTrackNavigator* nav = getNavigator (trackIndex);
    if (nav == nullptr) return;
    quantizer->schedule (nav, -1, q, currentTransportSample);
}

void SessionGrid::stopAll (Quantization q, long currentTransportSample)
{
    if (quantizer == nullptr) return;

    std::vector<LaunchQuantizer::SceneEntry> entries;
    entries.reserve (columns.size());
    for (const auto& c : columns)
        if (c.navigator != nullptr)
            entries.push_back ({ c.navigator, -1 });

    if (entries.empty()) return;
    quantizer->scheduleScene (entries.data(), (int) entries.size(),
                              q, currentTransportSample);
}

} // namespace synthlab::session
