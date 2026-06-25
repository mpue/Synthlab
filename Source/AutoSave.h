/*
  ==============================================================================

    AutoSave.h

    Crash-safe auto-save / recovery support.

    Auto-save writes a full project snapshot (module graph + tracks + audio
    config) into a dedicated recovery directory under ~/.Synthlab/recovery,
    completely separate from the user's own project files. It deliberately
    never touches the user's document state (current file path, dirty flag,
    tab title) — the goal is to be invisible during normal use.

    Lifecycle / crash detection (the same two-stage marker idea as
    CrashHandler):
      * SynthEditor::autoSave() writes the snapshot. It is triggered
        periodically from MainComponent's timer while the project has
        unsaved changes.
      * A clean shutdown (SynthlabApplication::shutdown) and every successful
        manual save call clearRecovery().
      * Therefore, if recovery files are still present at the next startup,
        the previous session ended unexpectedly (crash / power loss / kill)
        while there was unsaved work. MainComponent then offers to restore
        the snapshot.

    The recovery "project" mirrors a real save: a .slb plus _tracks.xml /
    _audio.xml siblings sharing this base name, so SynthEditor::openFile can
    restore it through the exact same pipeline as a normal project.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

namespace AutoSave
{
    inline juce::File getRecoveryDir()
    {
        const juce::String home =
            juce::File::getSpecialLocation (juce::File::userHomeDirectory).getFullPathName();
        juce::File dir (home + "/.Synthlab/recovery");
        if (! dir.exists())
            dir.createDirectory();
        return dir;
    }

    // Base file of the recovery snapshot. The _tracks.xml / _audio.xml
    // siblings and the optional edits/ folder live next to it.
    inline juce::File getRecoveryFile()
    {
        return getRecoveryDir().getChildFile ("recovery.slb");
    }

    inline juce::File getMetaFile()
    {
        return getRecoveryDir().getChildFile ("recovery.meta.xml");
    }

    // True if a recovery snapshot from a previous, unclean session is present.
    inline bool hasRecovery()
    {
        return getRecoveryFile().existsAsFile();
    }

    struct Meta
    {
        juce::String originalPath;   // empty for a never-saved project
        juce::String projectName;
        juce::String savedAt;        // ISO-8601, for display in the prompt
    };

    // Record where a restored project should be saved back to, so Ctrl+S after
    // a recovery writes to the user's real file rather than the recovery copy.
    inline void writeMeta (const juce::String& originalPath,
                           const juce::String& projectName)
    {
        juce::XmlElement xml ("SynthlabRecovery");
        xml.setAttribute ("originalPath", originalPath);
        xml.setAttribute ("projectName",  projectName);
        xml.setAttribute ("savedAt",      juce::Time::getCurrentTime().toISO8601 (true));
        xml.writeTo (getMetaFile());
    }

    inline Meta readMeta()
    {
        Meta m;
        std::unique_ptr<juce::XmlElement> xml (juce::XmlDocument::parse (getMetaFile()));
        if (xml != nullptr)
        {
            m.originalPath = xml->getStringAttribute ("originalPath");
            m.projectName  = xml->getStringAttribute ("projectName");
            m.savedAt      = xml->getStringAttribute ("savedAt");
        }
        return m;
    }

    // Remove every recovery artefact (snapshot, siblings, meta, edits/).
    // Called on a clean shutdown and after a successful manual save — both
    // mean there is nothing left to recover.
    inline void clearRecovery()
    {
        const juce::String home =
            juce::File::getSpecialLocation (juce::File::userHomeDirectory).getFullPathName();
        juce::File (home + "/.Synthlab/recovery").deleteRecursively();
    }
}
