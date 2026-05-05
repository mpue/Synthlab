/*
  ==============================================================================

    ProjectSnapshot.h

    Single Source of Truth fuer den Save-Pfad. Erfuellt Regel B aus
    docs/the-big-overhaul-status.md / the-big-overhaul.md:

      "saveProject ruft genau einmal currentSnapshot.load() auf und arbeitet
       danach nur auf dem geladenen shared_ptr<const ProjectSnapshot>."

    Ein Snapshot ist value-typed (kopierbar), traegt keinen Bezug auf den
    Live-State und wird ausschliesslich vom UI-/Background-Thread erzeugt.
    Allokation passiert NIE im Audio-Thread.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

#include <map>
#include <memory>
#include <atomic>

namespace synthlab {

struct ProjectSnapshot
{
    static constexpr int kCurrentVersion = 1;

    juce::String                          name;
    long                                  trackLength = 0;
    double                                sampleRate  = 44100.0;
    int                                   bufferSize  = 128;
    float                                 tempo       = 120.0f;
    bool                                  snapToGrid  = true;

    juce::ValueTree                       configTree; // ProjectConfig::getConfig()
    std::map<juce::String, juce::String>  audioFiles; // refId -> path

    // Vereinheitlicher Output ins XML-Format. Schliesst die Audio-Files
    // als zusaetzliche Kindern wie der bisherige save() ein.
    juce::ValueTree toValueTree() const
    {
        juce::ValueTree v = configTree.createCopy();
        if (! v.isValid())
            v = juce::ValueTree ("Project");

        v.setProperty ("snapshotVersion", kCurrentVersion, nullptr);

        juce::ValueTree audioFilesTree ("AudioFiles");
        for (const auto& kv : audioFiles)
        {
            juce::ValueTree e ("File");
            e.setProperty ("refId", kv.first,  nullptr);
            e.setProperty ("path",  kv.second, nullptr);
            audioFilesTree.addChild (e, -1, nullptr);
        }
        v.addChild (audioFilesTree, -1, nullptr);
        return v;
    }
};

// Atomarer Wrapper. Wir nutzen std::shared_ptr<const ...> + std::atomic_load
// statt std::atomic<std::shared_ptr<...>>, weil C++17 die atomic-Spezialisierung
// fuer shared_ptr noch braucht (sie ist deprecated in C++20). Funktional
// erfuellt es Regel B.
class SnapshotHolder
{
public:
    using Pointer = std::shared_ptr<const ProjectSnapshot>;

    SnapshotHolder()
    {
        // Mit leerem Snapshot starten, damit Loader nie null bekommen.
        std::atomic_store (&current, std::make_shared<const ProjectSnapshot>());
    }

    void publish (Pointer snap) noexcept
    {
        // Vorab allozierter Snapshot — niemals im Audio-Thread aufrufen.
        std::atomic_store (&current, std::move (snap));
    }

    Pointer load() const noexcept
    {
        return std::atomic_load (&current);
    }

private:
    Pointer current;
};

} // namespace synthlab
