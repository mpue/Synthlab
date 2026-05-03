#pragma once

#include "AutomationLane.h"

namespace synthlab { namespace automation {

// Sammlung der Lanes EINES Tracks.
//
// Eigenschaften (Akzeptanzkriterien aus the-big-overhaul.md):
//   * pro Plugin-Parameter genau eine Lane oder keine.
//   * findOrCreate(parameterId) ist idempotent.
//   * removeLane(id) laesst die anderen Lanes mit identischen Ids zurueck.
//   * Persistenz (toValueTree/fromValueTree) erhaelt Werte UND Ids.
//
// Versionierung: das ValueTree traegt eine "version"-Property. fromValueTree()
// ist tolerant gegenueber juengeren Versionen, in dem Sinn dass es nur die
// Felder liest, die es kennt — Migration ist Aufgabe von ProjectSnapshot.
class Model
{
public:
    static constexpr int kCurrentVersion = 1;

    Model() = default;

    // Idempotente Lane-Erzeugung. Existiert eine Lane mit der gegebenen
    // parameterId schon, wird sie zurueckgegeben — ohne deren Id zu aendern.
    Lane& findOrCreate (const juce::String& parameterId)
    {
        if (auto* existing = find (parameterId))
            return *existing;

        lanes.push_back (std::make_unique<Lane> (juce::Uuid(), parameterId));
        return *lanes.back();
    }

    Lane* find (const juce::String& parameterId) noexcept
    {
        for (auto& l : lanes)
            if (l->getParameterId() == parameterId) return l.get();
        return nullptr;
    }

    const Lane* find (const juce::String& parameterId) const noexcept
    {
        for (const auto& l : lanes)
            if (l->getParameterId() == parameterId) return l.get();
        return nullptr;
    }

    Lane* findById (const juce::Uuid& id) noexcept
    {
        for (auto& l : lanes)
            if (l->getId() == id) return l.get();
        return nullptr;
    }

    bool removeLane (const juce::Uuid& id) noexcept
    {
        const auto before = lanes.size();
        lanes.erase (std::remove_if (lanes.begin(), lanes.end(),
                                     [&] (const std::unique_ptr<Lane>& l)
                                     { return l->getId() == id; }),
                     lanes.end());
        return lanes.size() != before;
    }

    int  numLanes() const noexcept { return (int) lanes.size(); }
    bool isEmpty() const noexcept { return lanes.empty(); }
    void clear() noexcept { lanes.clear(); }

    Lane*       laneAt (int index)       noexcept { return (index >= 0 && index < (int) lanes.size()) ? lanes[(size_t) index].get() : nullptr; }
    const Lane* laneAt (int index) const noexcept { return (index >= 0 && index < (int) lanes.size()) ? lanes[(size_t) index].get() : nullptr; }

    juce::ValueTree toValueTree() const
    {
        juce::ValueTree v ("Automation");
        v.setProperty ("version", kCurrentVersion, nullptr);
        for (const auto& l : lanes)
            v.addChild (l->toValueTree(), -1, nullptr);
        return v;
    }

    void fromValueTree (const juce::ValueTree& v)
    {
        clear();
        if (! v.hasType ("Automation")) return;
        for (int i = 0; i < v.getNumChildren(); ++i)
        {
            auto child = v.getChild (i);
            if (! child.hasType ("Lane")) continue;
            lanes.push_back (std::make_unique<Lane> (Lane::fromValueTree (child)));
        }
    }

private:
    std::vector<std::unique_ptr<Lane>> lanes;
};

}}
