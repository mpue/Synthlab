/*
  ==============================================================================

    JavaScriptEditorComponent.cpp
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "JavaScriptEditorComponent.h"
#include "Tracks/TrackNavigator.h"
#include "Tracks/Track.h"
#include "Tracks/Region.h"
#include "Project/Project.h"
#include "Actions/MoveRegionAction.h"
#include "Actions/AddTrackAction.h"
#include "Actions/TrackDeleteAction.h"
#include "Actions/SetGainForRegionAction.h"
#include "Actions/AddNoteAction.h"
#include "Actions/DeleteNoteAction.h"
#include "Tracks/MidiRegion.h"
#include "Plugins/PluginManager.h"
#include "PluginModule.h"
#include "EditorComponent.h"
#include "TransportDisplayComponent.h"
#include "SynthEditor.h"

extern "C" {
    #include "duktape/duktape.h"
}

using namespace juce;

namespace
{
    // The print/console.log bridge runs synchronously on the same thread as
    // runScript(), so a thread-local pointer is enough to route output back
    // into the calling editor instance - no locking required.
    thread_local JavaScriptEditorComponent* currentEditor = nullptr;

    // maxArgs < 0 = alle Argumente nehmen (Default). alert() nutzt die obere
    // Grenze, um ein optionales Titel-Argument am Ende vom Message-Join
    // auszuschliessen.
    String collectAllArgsAsString (duk_context* ctx, int maxArgs = -1)
    {
        const int top = duk_get_top (ctx);
        const int n = (maxArgs < 0 || maxArgs > top) ? top : maxArgs;
        StringArray parts;

        for (int i = 0; i < n; ++i)
        {
            duk_dup (ctx, i);
            parts.add (String::fromUTF8 (duk_safe_to_string (ctx, -1)));
            duk_pop (ctx);
        }

        return parts.joinIntoString (" ");
    }

    duk_ret_t native_print (duk_context* ctx)
    {
        if (currentEditor != nullptr)
            currentEditor->appendOutput (collectAllArgsAsString (ctx) + "\n",
                                         Colours::lightgrey);
        return 0;
    }

    duk_ret_t native_console_log (duk_context* ctx)
    {
        if (currentEditor != nullptr)
            currentEditor->appendOutput (collectAllArgsAsString (ctx) + "\n",
                                         Colours::lightgrey);
        return 0;
    }

    duk_ret_t native_console_error (duk_context* ctx)
    {
        if (currentEditor != nullptr)
            currentEditor->appendOutput (collectAllArgsAsString (ctx) + "\n",
                                         Colours::tomato);
        return 0;
    }

    // --- alert() / prompt() ---------------------------------------------
    //
    // Browser-style modal dialogs. Both block the JS thread via
    // AlertWindow::runModalLoop - the script pauses until the user clicks
    // OK / Cancel. We are on the message thread (runScript is invoked from
    // a button or menu callback), so a nested modal loop is safe; audio
    // runs on its own thread and keeps streaming.
    //
    // runModalLoop is the "old" JUCE modal API and deprecated in favour of
    // async + callback, but synchronous semantics is exactly what a script
    // calling `var name = prompt("Name?")` expects - there's no way to
    // express that with a callback-only API without rebuilding duktape's
    // execution model around a task queue.

    duk_ret_t native_alert (duk_context* ctx)
    {
        // Wenn das letzte Argument ein String ist und mindestens zwei Args
        // uebergeben wurden, behandeln wir es als optionalen Fenstertitel.
        // So bleibt der Single-Arg-Fall `alert("foo")` unveraendert.
        const int nargs = duk_get_top (ctx);
        String title  = "Synthlab Script";
        int msgArgs = nargs;
        if (nargs >= 2 && duk_is_string (ctx, nargs - 1))
        {
            title = String::fromUTF8 (duk_get_string (ctx, nargs - 1));
            msgArgs = nargs - 1;
        }

        const String msg = collectAllArgsAsString (ctx, msgArgs);

        AlertWindow w (title, msg, AlertWindow::InfoIcon);
        // Auf das App-LookAndFeel umstellen, damit Script-Dialoge optisch nicht
        // aus dem Rahmen fallen (vgl. MainComponent::showClaudeApiKeyDialog).
        if (auto* laf = Project::getInstance()->getLookAndFeel())
            w.setLookAndFeel (laf);
        w.addButton ("OK", 1, KeyPress (KeyPress::returnKey),
                              KeyPress (KeyPress::escapeKey));
        w.runModalLoop();
        return 0;
    }

    duk_ret_t native_prompt (duk_context* ctx)
    {
        const int nargs = duk_get_top (ctx);
        const String message      = nargs > 0 ? String::fromUTF8 (duk_safe_to_string (ctx, 0)) : String();
        const String defaultValue = nargs > 1 ? String::fromUTF8 (duk_safe_to_string (ctx, 1)) : String();
        const String title        = nargs > 2 ? String::fromUTF8 (duk_safe_to_string (ctx, 2))
                                              : String ("Synthlab Script");

        AlertWindow w (title, message, AlertWindow::QuestionIcon);
        if (auto* laf = Project::getInstance()->getLookAndFeel())
            w.setLookAndFeel (laf);
        w.addTextEditor ("input", defaultValue, String(), false);
        w.addButton ("OK",     1, KeyPress (KeyPress::returnKey));
        w.addButton ("Cancel", 0, KeyPress (KeyPress::escapeKey));

        const int result = w.runModalLoop();

        if (result == 1)
        {
            const String text = w.getTextEditorContents ("input");
            duk_push_string (ctx, text.toRawUTF8());
        }
        else
        {
            // Browser convention: prompt() returns null when the user
            // cancels. Scripts can check via `if (s === null) ...`.
            duk_push_null (ctx);
        }
        return 1;
    }

    // --- synthlab.transport bridge --------------------------------------
    //
    // runScript() is invoked from a Button::Listener callback, so we are on
    // the message thread when these natives fire. TrackNavigator's play /
    // stop / setPosition entry points are already called from key handlers
    // on the same thread, so calling them directly here is safe - no need
    // for callAsync.

    TrackNavigator* currentNavigator()
    {
        return currentEditor != nullptr ? currentEditor->getTransportNavigator()
                                        : nullptr;
    }

    duk_ret_t native_transport_play (duk_context*)
    {
        if (auto* n = currentNavigator()) n->setPlaying (true);
        return 0;
    }

    duk_ret_t native_transport_stop (duk_context*)
    {
        if (auto* n = currentNavigator()) n->setPlaying (false);
        return 0;
    }

    duk_ret_t native_transport_toggle (duk_context*)
    {
        if (auto* n = currentNavigator()) n->togglePlayStop();
        return 0;
    }

    duk_ret_t native_transport_isPlaying (duk_context* ctx)
    {
        auto* n = currentNavigator();
        duk_push_boolean (ctx, n != nullptr && n->isPlaying());
        return 1;
    }

    duk_ret_t native_transport_getPosition (duk_context* ctx)
    {
        auto* n = currentNavigator();
        duk_push_number (ctx, n != nullptr ? n->getPosition() : 0.0);
        return 1;
    }

    duk_ret_t native_transport_setPosition (duk_context* ctx)
    {
        if (auto* n = currentNavigator())
            n->setPosition (duk_to_number (ctx, 0));
        return 0;
    }

    duk_ret_t native_transport_getBpm (duk_context* ctx)
    {
        duk_push_number (ctx, (double) Project::getInstance()->getTempo());
        return 1;
    }

    duk_ret_t native_transport_setBpm (duk_context* ctx)
    {
        const double v = duk_to_number (ctx, 0);

        // TrackNavigator's BPM-edit path (TransportDisplayComponent) clamps
        // to a sensible range; mirror that here so a stray script can't push
        // the project into a state the UI editor would reject.
        if (v >= 20.0 && v <= 999.0)
            Project::getInstance()->setTempo ((float) v);

        return 0;
    }

    // Global record state - the audio block in MainComponent gates note
    // capture on navigator->isRecording(). Used after arming individual
    // tracks (synthlab.tracks.setRecording) to actually start writing notes.
    duk_ret_t native_transport_isRecording (duk_context* ctx)
    {
        auto* n = currentNavigator();
        duk_push_boolean (ctx, n != nullptr && n->isRecording());
        return 1;
    }

    duk_ret_t native_transport_setRecording (duk_context* ctx)
    {
        if (auto* n = currentNavigator())
            n->setRecording (duk_to_boolean (ctx, 0) != 0);
        return 0;
    }

    // Metronome flag lives on the TransportDisplayComponent - the audio
    // block reads it via that pointer (MainComponent::getNextAudioBlock).
    // The click also requires navigator->isRecording() == true to actually
    // generate samples; metronome=on without recording is silent.
    TransportDisplayComponent* currentTransportDisplay()
    {
        return currentEditor != nullptr ? currentEditor->getTransportDisplay()
                                        : nullptr;
    }

    duk_ret_t native_transport_getMetronome (duk_context* ctx)
    {
        auto* td = currentTransportDisplay();
        duk_push_boolean (ctx, td != nullptr && td->isMetronomeActive());
        return 1;
    }

    duk_ret_t native_transport_setMetronome (duk_context* ctx)
    {
        if (auto* td = currentTransportDisplay())
            td->setMetronomeState (duk_to_boolean (ctx, 0) != 0);
        return 0;
    }

    // --- synthlab.tracks / synthlab.regions -----------------------------
    //
    // These are read-only views over the current TrackNavigator state.
    // Out-of-range indices return undefined / sensible defaults instead of
    // throwing - duk_error uses longjmp which would skip C++ destructors
    // (e.g. the std::vector<Track*> returned by getTracks()), so we avoid
    // throwing from these natives.

    Track* trackAt (int idx)
    {
        auto* n = currentNavigator();
        if (n == nullptr) return nullptr;
        auto tracks = n->getTracks();
        if (idx < 0 || idx >= (int) tracks.size()) return nullptr;
        return tracks[(size_t) idx];
    }

    Region* regionAt (int trackIdx, int regionIdx)
    {
        auto* t = trackAt (trackIdx);
        if (t == nullptr) return nullptr;
        auto regions = t->getRegions();
        if (regionIdx < 0 || regionIdx >= (int) regions.size()) return nullptr;
        return regions[(size_t) regionIdx];
    }

    MidiRegion* midiRegionAt (int trackIdx, int regionIdx)
    {
        return dynamic_cast<MidiRegion*> (regionAt (trackIdx, regionIdx));
    }

    // Pair note-on / note-off messages with the same FIFO logic as
    // PianoRollComponent::findNoteOffForNoteOn: per (channel, pitch) group,
    // sort on/off lists by timestamp and pair k-th-on with k-th-off. Returns
    // pairs sorted by note-on time so JS callers see a stable index ordering.
    // off may be nullptr when a note-on has no matching note-off (corrupt
    // data); getNote reports lengthSec=0 in that case, deleteNote refuses.
    std::vector<std::pair<juce::MidiMessage*, juce::MidiMessage*>>
    collectNotePairs (MidiRegion* mr)
    {
        std::vector<std::pair<juce::MidiMessage*, juce::MidiMessage*>> out;
        if (mr == nullptr) return out;

        auto& messages = mr->getMidiMessageList();

        // Group by (channel, pitch) once so we don't rescan the whole list per
        // note-on. Map value = (ons sorted by time, offs sorted by time).
        std::map<std::pair<int,int>,
                 std::pair<std::vector<juce::MidiMessage*>,
                           std::vector<juce::MidiMessage*>>> groups;

        for (auto* m : messages)
        {
            if (m == nullptr) continue;
            if (! (m->isNoteOn() || m->isNoteOff())) continue;
            auto& g = groups[{ m->getChannel(), m->getNoteNumber() }];
            (m->isNoteOn() ? g.first : g.second).push_back (m);
        }

        auto byTime = [] (juce::MidiMessage* a, juce::MidiMessage* b) {
            return a->getTimeStamp() < b->getTimeStamp();
        };
        for (auto& kv : groups)
        {
            std::stable_sort (kv.second.first.begin(),  kv.second.first.end(),  byTime);
            std::stable_sort (kv.second.second.begin(), kv.second.second.end(), byTime);
            for (size_t i = 0; i < kv.second.first.size(); ++i)
            {
                juce::MidiMessage* off = i < kv.second.second.size()
                                            ? kv.second.second[i] : nullptr;
                out.emplace_back (kv.second.first[i], off);
            }
        }

        std::stable_sort (out.begin(), out.end(),
            [] (const auto& a, const auto& b) {
                return a.first->getTimeStamp() < b.first->getTimeStamp();
            });
        return out;
    }

    const char* trackTypeName (Track::Type type)
    {
        switch (type)
        {
            case Track::AUDIO:      return "audio";
            case Track::MIDI:       return "midi";
            case Track::INSTRUMENT: return "instrument";
        }
        return "unknown";
    }

    duk_ret_t native_tracks_count (duk_context* ctx)
    {
        auto* n = currentNavigator();
        duk_push_int (ctx, n != nullptr ? (int) n->getTracks().size() : 0);
        return 1;
    }

    duk_ret_t native_tracks_list (duk_context* ctx)
    {
        auto* n = currentNavigator();
        duk_idx_t arr = duk_push_array (ctx);
        if (n != nullptr)
        {
            auto tracks = n->getTracks();
            for (size_t i = 0; i < tracks.size(); ++i)
            {
                duk_push_string (ctx, tracks[i]->getName().toRawUTF8());
                duk_put_prop_index (ctx, arr, (duk_uarridx_t) i);
            }
        }
        return 1;
    }

    duk_ret_t native_tracks_getName (duk_context* ctx)
    {
        auto* t = trackAt (duk_to_int (ctx, 0));
        if (t == nullptr) { duk_push_undefined (ctx); return 1; }
        duk_push_string (ctx, t->getName().toRawUTF8());
        return 1;
    }

    duk_ret_t native_tracks_getType (duk_context* ctx)
    {
        auto* t = trackAt (duk_to_int (ctx, 0));
        if (t == nullptr) { duk_push_undefined (ctx); return 1; }
        duk_push_string (ctx, trackTypeName (t->getType()));
        return 1;
    }

    duk_ret_t native_tracks_isMute (duk_context* ctx)
    {
        auto* t = trackAt (duk_to_int (ctx, 0));
        duk_push_boolean (ctx, t != nullptr && t->isMute());
        return 1;
    }

    duk_ret_t native_tracks_isSolo (duk_context* ctx)
    {
        auto* t = trackAt (duk_to_int (ctx, 0));
        duk_push_boolean (ctx, t != nullptr && t->isSolo());
        return 1;
    }

    duk_ret_t native_tracks_getGain (duk_context* ctx)
    {
        auto* t = trackAt (duk_to_int (ctx, 0));
        duk_push_number (ctx, t != nullptr ? (double) t->getGain() : 0.0);
        return 1;
    }

    duk_ret_t native_tracks_getVolume (duk_context* ctx)
    {
        auto* t = trackAt (duk_to_int (ctx, 0));
        duk_push_number (ctx, t != nullptr ? (double) t->getVolume() : 0.0);
        return 1;
    }

    duk_ret_t native_tracks_getPan (duk_context* ctx)
    {
        auto* t = trackAt (duk_to_int (ctx, 0));
        duk_push_number (ctx, t != nullptr ? (double) t->getPan() : 0.0);
        return 1;
    }

    duk_ret_t native_tracks_isSelected (duk_context* ctx)
    {
        auto* t = trackAt (duk_to_int (ctx, 0));
        duk_push_boolean (ctx, t != nullptr && t->isSelected());
        return 1;
    }

    duk_ret_t native_tracks_regionCount (duk_context* ctx)
    {
        auto* t = trackAt (duk_to_int (ctx, 0));
                duk_push_int (ctx, t != nullptr ? (int) t->getRegions().size() : 0);
        return 1;
    }

    // Track-level "armed for record" flag - scripts toggle this before
    // flipping the global transport record state (synthlab.transport.
    // setRecording), which is the gate the audio thread reads. Lives under
    // tracks.* because it IS per-track state; not under actions.* because
    // it's not undoable.
    duk_ret_t native_tracks_isRecording (duk_context* ctx)
    {
        auto* t = trackAt (duk_to_int (ctx, 0));
        duk_push_boolean (ctx, t != nullptr && t->isRecording());
        return 1;
    }

    duk_ret_t native_tracks_setRecording (duk_context* ctx)
    {
        auto* t = trackAt (duk_to_int (ctx, 0));
        const bool armed = duk_to_boolean (ctx, 1) != 0;
        if (t == nullptr) { duk_push_boolean (ctx, false); return 1; }
        t->setRecording (armed);
        duk_push_boolean (ctx, true);
        return 1;
    }

    // Region helpers - start/length exposed in seconds using the project
    // sample rate (same one TrackNavigator.setPosition() consumes), so a
    // script can round-trip via synthlab.transport.setPosition() without
    // unit conversion.
    double projectSampleRate()
    {
        const double sr = Project::getInstance()->getSampleRate();
        return sr > 0.0 ? sr : 44100.0;
    }

    duk_ret_t native_regions_getName (duk_context* ctx)
    {
        auto* r = regionAt (duk_to_int (ctx, 0), duk_to_int (ctx, 1));
        if (r == nullptr) { duk_push_undefined (ctx); return 1; }
        duk_push_string (ctx, r->getName().toRawUTF8());
        return 1;
    }

    duk_ret_t native_regions_getStart (duk_context* ctx)
    {
        auto* r = regionAt (duk_to_int (ctx, 0), duk_to_int (ctx, 1));
        duk_push_number (ctx, r != nullptr ? (double) r->getSampleOffset() / projectSampleRate()
                                           : 0.0);
        return 1;
    }

    duk_ret_t native_regions_getLength (duk_context* ctx)
    {
        auto* r = regionAt (duk_to_int (ctx, 0), duk_to_int (ctx, 1));
        duk_push_number (ctx, r != nullptr ? (double) r->getNumSamples() / projectSampleRate()
                                           : 0.0);
        return 1;
    }
    
    duk_ret_t native_regions_isSelected (duk_context* ctx)
    {
        auto* r = regionAt (duk_to_int (ctx, 0), duk_to_int (ctx, 1));
        duk_push_boolean (ctx, r != nullptr && r->isSelected());
        return 1;
    }

    duk_ret_t native_regions_isLoop (duk_context* ctx)
    {
        auto* r = regionAt (duk_to_int (ctx, 0), duk_to_int (ctx, 1));
        duk_push_boolean (ctx, r != nullptr && r->isLoop());
        return 1;
    }

    duk_ret_t native_regions_getGain (duk_context* ctx)
    {
        auto* r = regionAt (duk_to_int (ctx, 0), duk_to_int (ctx, 1));
        // Region base returns 1.0 (MIDI/instrument); AudioRegion overrides.
        duk_push_number (ctx, r != nullptr ? (double) r->getGain() : 1.0);
        return 1;
    }

    // --- MIDI note read API --------------------------------------------
    //
    // Indexes are into the FIFO-paired note-on list (sorted by start time).
    // Non-MIDI regions report 0 / undefined.

    duk_ret_t native_regions_noteCount (duk_context* ctx)
    {
        auto* mr = midiRegionAt (duk_to_int (ctx, 0), duk_to_int (ctx, 1));
        if (mr == nullptr) { duk_push_int (ctx, 0); return 1; }

        int count = 0;
        for (auto* m : mr->getMidiMessageList())
            if (m != nullptr && m->isNoteOn()) ++count;
        duk_push_int (ctx, count);
        return 1;
    }

    duk_ret_t native_regions_getNote (duk_context* ctx)
    {
        auto* mr = midiRegionAt (duk_to_int (ctx, 0), duk_to_int (ctx, 1));
        const int idx = duk_to_int (ctx, 2);
        if (mr == nullptr) { duk_push_undefined (ctx); return 1; }

        auto pairs = collectNotePairs (mr);
        if (idx < 0 || idx >= (int) pairs.size())
        { duk_push_undefined (ctx); return 1; }

        auto* on  = pairs[(size_t) idx].first;
        auto* off = pairs[(size_t) idx].second;

        const double sr = projectSampleRate();
        // Buffer-relative timestamps subtract sourceStartSample so that
        // startSec=0 maps to the visible left edge of the region - matching
        // the addNote convention (region-relative time).
        const double srcStart  = (double) mr->getSourceStartSample();
        const double startSec  = ((double) on->getTimeStamp() - srcStart) / sr;
        const double lengthSec = off != nullptr
            ? ((double) off->getTimeStamp() - (double) on->getTimeStamp()) / sr
            : 0.0;

        duk_push_object (ctx);
        duk_push_int    (ctx, on->getNoteNumber());   duk_put_prop_string (ctx, -2, "pitch");
        duk_push_int    (ctx, on->getVelocity());     duk_put_prop_string (ctx, -2, "velocity");
        duk_push_int    (ctx, on->getChannel());      duk_put_prop_string (ctx, -2, "channel");
        duk_push_number (ctx, startSec);              duk_put_prop_string (ctx, -2, "startSec");
        duk_push_number (ctx, lengthSec);             duk_put_prop_string (ctx, -2, "lengthSec");
        return 1;
    }

    // --- synthlab.plugins -----------------------------------------------
    //
    // Reads the same plugin list the Instruments tab and the plugin-picker
    // dialog see - populated by PluginManager::scanPlugins() at startup
    // from $HOME/.Synthlab/plugins or wherever the scanner deposits the
    // .xml descriptors. Empty list usually means "scan never ran" or "no
    // instrument plugins installed".

    duk_ret_t native_plugins_list (duk_context* ctx)
    {
        duk_idx_t arr = duk_push_array (ctx);
        const auto names = PluginManager::getInstance()->getAvailablePlugins();
        for (size_t i = 0; i < names.size(); ++i)
        {
            duk_push_string (ctx, names[i].toRawUTF8());
            duk_put_prop_index (ctx, arr, (duk_uarridx_t) i);
        }
        return 1;
    }

    // --- synthlab.ui ----------------------------------------------------
    //
    // UI side-effects: not undoable, not state-mutating from the user's POV.
    // Kept separate from synthlab.actions so the "every action is undoable"
    // contract there stays intact.

    duk_ret_t native_ui_openPlugin (duk_context* ctx)
    {
        auto* t = trackAt (duk_to_int (ctx, 0));
        if (t == nullptr) { duk_push_boolean (ctx, false); return 1; }

        // Only instrument tracks own a PluginModule. Audio/MIDI tracks
        // return nullptr - silently fail with `false`.
        auto* pm = t->getPluginModule();
        if (pm == nullptr) { duk_push_boolean (ctx, false); return 1; }

        pm->openPluginWindow();
        duk_push_boolean (ctx, true);
        return 1;
    }

    // Brings the Tracks tab to front in the editor's tab bar (and creates
    // the navigator if it hasn't been opened yet). MainComponent's
    // menuItemSelected(201) does the same thing - we just bypass the menu.
    duk_ret_t native_ui_openTrackView (duk_context* ctx)
    {
        if (currentEditor == nullptr) { duk_push_boolean (ctx, false); return 1; }
        auto* ev = currentEditor->getEditorView();
        if (ev == nullptr) { duk_push_boolean (ctx, false); return 1; }
        ev->OpenTrackView();
        duk_push_boolean (ctx, true);
        return 1;
    }

    // --- synthlab.actions -----------------------------------------------
    //
    // All writes go through juce::UndoableAction so a single Cmd-Z reverses
    // the script's effects exactly like a UI edit would. Each scripted call
    // is its own transaction.

    duk_ret_t native_actions_moveRegion (duk_context* ctx)
    {
        auto* r = regionAt (duk_to_int (ctx, 0), duk_to_int (ctx, 1));
        const double deltaSeconds = duk_to_number (ctx, 2);

        if (r == nullptr) { duk_push_boolean (ctx, false); return 1; }

        const long oldOffset    = r->getSampleOffset();
        const long deltaSamples = (long) std::llround (deltaSeconds * projectSampleRate());
        const long newOffset    = std::max<long> (0, oldOffset + deltaSamples);

        if (newOffset == oldOffset) { duk_push_boolean (ctx, true); return 1; }

        auto* um = Project::getInstance()->getUndoManager();
        um->beginNewTransaction();
        const bool ok = um->perform (new MoveRegionAction (r, oldOffset, newOffset));
        duk_push_boolean (ctx, ok);
        return 1;
    }

    duk_ret_t native_actions_addTrackInternal (duk_context* ctx,
                                                AddTrackAction::Type type,
                                                const juce::String& instrumentName = {})
    {
        if (currentEditor == nullptr) { duk_push_int (ctx, -1); return 1; }

        auto* nav = currentEditor->getTransportNavigator();
        auto* ed  = currentEditor->getSynthEditor();
        if (nav == nullptr || ed == nullptr) { duk_push_int (ctx, -1); return 1; }

        auto* action = new AddTrackAction (ed, nav, type, instrumentName);
        auto* um     = Project::getInstance()->getUndoManager();
        um->beginNewTransaction();
        if (! um->perform (action)) { duk_push_int (ctx, -1); return 1; }

        // perform() owns the new track - find its index for the script.
        Track* created = action->getTrack();
        auto tracks = nav->getTracks();
        int found = -1;
        for (size_t i = 0; i < tracks.size(); ++i)
            if (tracks[i] == created) { found = (int) i; break; }

        // Mirror MainComponent's add-track menu: editor needs a repaint so
        // the new strip shows up immediately.
        if (auto* edComp = dynamic_cast<juce::Component*> (ed))
            edComp->repaint();

        duk_push_int (ctx, found);
        return 1;
    }

    duk_ret_t native_actions_addAudioTrack (duk_context* ctx)
    {
        return native_actions_addTrackInternal (ctx, AddTrackAction::Type::Audio);
    }

    duk_ret_t native_actions_addMidiTrack (duk_context* ctx)
    {
        return native_actions_addTrackInternal (ctx, AddTrackAction::Type::Midi);
    }

    duk_ret_t native_actions_addInstrumentTrack (duk_context* ctx)
    {
        // Plugin name is required - SynthEditor::addVirtualInstrumentTrack()
        // (no-arg version) opens the plugin picker, which doesn't make sense
        // from a script. Always go through the named overload via the action.
        if (duk_get_top (ctx) < 1) { duk_push_int (ctx, -1); return 1; }
        const String pluginName = String::fromUTF8 (duk_safe_to_string (ctx, 0));
        if (pluginName.isEmpty()) { duk_push_int (ctx, -1); return 1; }

        return native_actions_addTrackInternal (ctx, AddTrackAction::Type::Instrument, pluginName);
    }

    // addMidiClip: NOT undoable in this iteration. Track::addMidiRegion
    // pushes the new region directly to the track's regions vector with
    // listener/zoom/thumbnail wiring; an undoable wrapper needs a Track-
    // side removeRegion helper that detaches without deleting (so redo can
    // reattach). Left as a follow-up - the typical use case for this API
    // is "create empty clip -> arm -> record into it", where the user almost
    // never wants to undo the clip-add itself.
    duk_ret_t native_actions_addMidiClip (duk_context* ctx)
    {
        auto* t = trackAt (duk_to_int (ctx, 0));
        const double startSec  = duk_to_number (ctx, 1);
        const double lengthSec = duk_to_number (ctx, 2);
        if (t == nullptr || lengthSec <= 0.0) { duk_push_int (ctx, -1); return 1; }

        const double sr = projectSampleRate();
        const long samplePos = (long) std::llround (startSec  * sr);
        const long sampleLen = (long) std::llround (lengthSec * sr);

        // Plain ternary - juce::jmax<long> pulls in juce_dsp's SIMDRegister<long>
        // template, which has no specialization and breaks the build.
        const long clampedStart = samplePos < 0 ? 0L : samplePos;
        auto* region = t->addMidiRegion (sr, clampedStart, sampleLen);
        if (region == nullptr) { duk_push_int (ctx, -1); return 1; }

        // Find the index of the new region within the track for return.
        const auto regs = t->getRegions();
        int found = -1;
        for (size_t i = 0; i < regs.size(); ++i)
            if (regs[i] == region) { found = (int) i; break; }
        duk_push_int (ctx, found);
        return 1;
    }

    // --- MIDI note write API (undoable) --------------------------------
    //
    // startSec is RELATIVE to the region's left edge (sec 0 = left edge),
    // matching synthlab.regions.getNote. Adds a note-on/note-off pair via
    // AddNoteAction so a single Cmd-Z removes both.

    duk_ret_t native_actions_addNote (duk_context* ctx)
    {
        auto* mr = midiRegionAt (duk_to_int (ctx, 0), duk_to_int (ctx, 1));
        if (mr == nullptr) { duk_push_boolean (ctx, false); return 1; }

        const int pitch        = duk_to_int (ctx, 2);
        const int velocity     = duk_to_int (ctx, 3);
        const double startSec  = duk_to_number (ctx, 4);
        const double lengthSec = duk_to_number (ctx, 5);
        const int channel      = duk_get_top (ctx) > 6 ? duk_to_int (ctx, 6) : 1;

        if (pitch    < 0 || pitch    > 127
         || velocity < 0 || velocity > 127
         || channel  < 1 || channel  > 16
         || lengthSec <= 0.0)
        { duk_push_boolean (ctx, false); return 1; }

        const double sr = projectSampleRate();
        const juce::int64 srcStart = (juce::int64) mr->getSourceStartSample();
        const juce::int64 onTs  = srcStart + (juce::int64) std::llround (startSec  * sr);
        const juce::int64 offTs = onTs     + (juce::int64) std::llround (lengthSec * sr);

        if (onTs < 0) { duk_push_boolean (ctx, false); return 1; }

        juce::MidiMessage on  = juce::MidiMessage::noteOn  (channel, pitch, (juce::uint8) velocity);
        juce::MidiMessage off = juce::MidiMessage::noteOff (channel, pitch);
        on .setTimeStamp ((double) onTs);
        off.setTimeStamp ((double) offTs);

        auto* um = Project::getInstance()->getUndoManager();
        um->beginNewTransaction();
        const bool ok = um->perform (new AddNoteAction (mr, on, off));
        duk_push_boolean (ctx, ok);
        return 1;
    }

    duk_ret_t native_actions_deleteNote (duk_context* ctx)
    {
        auto* mr = midiRegionAt (duk_to_int (ctx, 0), duk_to_int (ctx, 1));
        const int idx = duk_to_int (ctx, 2);
        if (mr == nullptr) { duk_push_boolean (ctx, false); return 1; }

        auto pairs = collectNotePairs (mr);
        if (idx < 0 || idx >= (int) pairs.size())
        { duk_push_boolean (ctx, false); return 1; }

        auto* on  = pairs[(size_t) idx].first;
        auto* off = pairs[(size_t) idx].second;
        // No off -> orphaned note-on, can't round-trip undo cleanly; refuse.
        if (on == nullptr || off == nullptr)
        { duk_push_boolean (ctx, false); return 1; }

        juce::MidiMessage onCopy  (*on);
        juce::MidiMessage offCopy (*off);

        auto* um = Project::getInstance()->getUndoManager();
        um->beginNewTransaction();
        const bool ok = um->perform (new DeleteNoteAction (mr, onCopy, offCopy));
        duk_push_boolean (ctx, ok);
        return 1;
    }

    duk_ret_t native_actions_setRegionGain (duk_context* ctx)
    {
        auto* r = regionAt (duk_to_int (ctx, 0), duk_to_int (ctx, 1));
        const double newGain = duk_to_number (ctx, 2);

        if (r == nullptr) { duk_push_boolean (ctx, false); return 1; }

        const float oldGain = r->getGain();
        const float clamped = juce::jlimit (0.0f, 4.0f, (float) newGain);
        if (oldGain == clamped) { duk_push_boolean (ctx, true); return 1; }

        auto* um = Project::getInstance()->getUndoManager();
        um->beginNewTransaction();
        const bool ok = um->perform (new SetGainForRegionAction (r, oldGain, clamped));
        duk_push_boolean (ctx, ok);
        return 1;
    }

    // Undo/Redo - drive the same UndoManager that Cmd-Z uses. Returns the
    // bool from juce::UndoManager so a test can assert "nothing left to undo"
    // by checking the return value. Pair these with addTrack/etc. for
    // roundtrip tests: state -> mutate -> undo -> assert state restored.
    duk_ret_t native_actions_undo (duk_context* ctx)
    {
        auto* um = Project::getInstance()->getUndoManager();
        duk_push_boolean (ctx, um != nullptr && um->undo());
        return 1;
    }

    duk_ret_t native_actions_redo (duk_context* ctx)
    {
        auto* um = Project::getInstance()->getUndoManager();
        duk_push_boolean (ctx, um != nullptr && um->redo());
        return 1;
    }

    // --- synthlab.test -------------------------------------------------
    //
    // Assertion helpers. All three raise a JS Error via duk_error when the
    // expected condition is not met, which the existing runScriptText path
    // catches and shows in red in the output panel. Scripts that wrap
    // assertions in try/catch can keep going after a failure; bare scripts
    // stop at the first failed assertion (intentional - matches the
    // assert-style convention in most test libraries).
    //
    // We use only Duktape-owned C strings (duk_safe_to_string) on the
    // duk_error path; no C++ String/RAII objects are alive when the longjmp
    // fires.

    duk_ret_t native_test_assert (duk_context* ctx)
    {
        if (duk_to_boolean (ctx, 0) == 0)
        {
            const char* msg = duk_get_top (ctx) > 1
                                ? duk_safe_to_string (ctx, 1)
                                : "assertion failed";
            duk_error (ctx, DUK_ERR_ERROR, "%s", msg);
        }
        return 0;
    }

    duk_ret_t native_test_assertEquals (duk_context* ctx)
    {
        // strict equality via duktape's === semantics
        duk_dup (ctx, 0);
        duk_dup (ctx, 1);
        const bool equal = duk_strict_equals (ctx, -2, -1) != 0;
        duk_pop_2 (ctx);

        if (! equal)
        {
            const char* actual   = duk_safe_to_string (ctx, 0);
            const char* expected = duk_safe_to_string (ctx, 1);
            const char* msg      = duk_get_top (ctx) > 2
                                       ? duk_safe_to_string (ctx, 2)
                                       : "assertEquals failed";
            duk_error (ctx, DUK_ERR_ERROR,
                       "%s: expected %s but got %s",
                       msg, expected, actual);
        }
        return 0;
    }

    duk_ret_t native_test_assertThrows (duk_context* ctx)
    {
        if (! duk_is_callable (ctx, 0))
            duk_error (ctx, DUK_ERR_TYPE_ERROR,
                       "assertThrows: first argument must be a function");

        duk_dup (ctx, 0);
        const int rc = duk_pcall (ctx, 0);

        if (rc == DUK_EXEC_SUCCESS)
        {
            const char* msg = duk_get_top (ctx) > 1
                                ? duk_safe_to_string (ctx, 1)
                                : "assertThrows: function did not throw";
            // No leak risk: duktape strings, no C++ RAII on stack.
            duk_error (ctx, DUK_ERR_ERROR, "%s", msg);
        }

        duk_pop (ctx);  // discard the captured error from pcall
        return 0;
    }

    duk_ret_t native_actions_deleteTrack (duk_context* ctx)
    {
        auto* t = trackAt (duk_to_int (ctx, 0));
        if (t == nullptr || currentEditor == nullptr) { duk_push_boolean (ctx, false); return 1; }

        auto* nav = currentEditor->getTransportNavigator();
        auto* ed  = currentEditor->getSynthEditor();
        if (nav == nullptr || ed == nullptr) { duk_push_boolean (ctx, false); return 1; }

        auto* um = Project::getInstance()->getUndoManager();
        um->beginNewTransaction();
        const bool ok = um->perform (new TrackDeleteAction (nav, ed, t));
        duk_push_boolean (ctx, ok);
        return 1;
    }

    // --- synthlab.project ----------------------------------------------
    //
    // Save / load / clear for the whole project. Lets test macros drive
    // full save/load roundtrips - the most valuable regression check for a
    // DAW since persistence is where the most schema-drift bugs hide.
    //
    // save()  writes the current project (.slb + sibling _tracks.xml +
    //         _audio.xml) to the given path, no dialog. Mutates the
    //         project's "current file path" - same as the user clicking
    //         Save As.
    // load()  replaces the current scene from the given path, no
    //         "discard dirty?" prompt. Tests that care should call
    //         clear() first to start from a known state.
    // clear() = File > New, no dirty prompt.

    duk_ret_t native_project_save (duk_context* ctx)
    {
        if (currentEditor == nullptr) { duk_push_boolean (ctx, false); return 1; }
        auto* ed = currentEditor->getSynthEditor();
        if (ed == nullptr) { duk_push_boolean (ctx, false); return 1; }

        if (duk_get_top (ctx) < 1) { duk_push_boolean (ctx, false); return 1; }
        const String path = String::fromUTF8 (duk_safe_to_string (ctx, 0));
        if (path.isEmpty()) { duk_push_boolean (ctx, false); return 1; }

        duk_push_boolean (ctx, ed->saveFileTo (File (path)));
        return 1;
    }

    duk_ret_t native_project_load (duk_context* ctx)
    {
        if (currentEditor == nullptr) { duk_push_boolean (ctx, false); return 1; }
        auto* ed = currentEditor->getSynthEditor();
        if (ed == nullptr) { duk_push_boolean (ctx, false); return 1; }

        if (duk_get_top (ctx) < 1) { duk_push_boolean (ctx, false); return 1; }
        const String path = String::fromUTF8 (duk_safe_to_string (ctx, 0));
        if (path.isEmpty()) { duk_push_boolean (ctx, false); return 1; }

        duk_push_boolean (ctx, ed->openFileNoPrompt (File (path)));
        return 1;
    }

    duk_ret_t native_project_clear (duk_context* ctx)
    {
        if (currentEditor == nullptr) { duk_push_boolean (ctx, false); return 1; }
        auto* nav = currentEditor->getTransportNavigator();
        if (nav == nullptr) { duk_push_boolean (ctx, false); return 1; }

        // Narrow clear: just drop all tracks from the navigator. The full
        // SynthEditor::cleanUp pathway currently has a latent UAF on AuxOut's
        // nameEditor TextEditor when invoked from a script context (visible
        // when project.save runs immediately before clear). Until that root
        // cause is fixed, we keep clear() in the scope that matters for the
        // roundtrip test: synthlab.tracks.count() must go to zero. Root
        // modules (AudioOut/AuxOut/BusOut etc.) stay alive — they get
        // overwritten by the next project.load.
        nav->clearTracks();

        duk_push_boolean (ctx, true);
        return 1;
    }

    duk_ret_t native_project_getCurrentPath (duk_context* ctx)
    {
        const String p = Project::getInstance()->getCurrentFilePath();
        duk_push_string (ctx, p.toRawUTF8());
        return 1;
    }

    duk_ret_t native_project_isDirty (duk_context* ctx)
    {
        if (currentEditor == nullptr) { duk_push_boolean (ctx, false); return 1; }
        auto* ed = currentEditor->getSynthEditor();
        duk_push_boolean (ctx, ed != nullptr && ed->isDirty());
        return 1;
    }

    void registerSynthlabApi (duk_context* ctx)
    {
        duk_push_object (ctx);                                     // synthlab
        {
            // --- transport ----------------------------------------------
            duk_push_object (ctx);
            duk_push_c_function (ctx, native_transport_play,        0);
            duk_put_prop_string (ctx, -2, "play");
            duk_push_c_function (ctx, native_transport_stop,        0);
            duk_put_prop_string (ctx, -2, "stop");
            duk_push_c_function (ctx, native_transport_toggle,      0);
            duk_put_prop_string (ctx, -2, "toggle");
            duk_push_c_function (ctx, native_transport_isPlaying,   0);
            duk_put_prop_string (ctx, -2, "isPlaying");
            duk_push_c_function (ctx, native_transport_getPosition, 0);
            duk_put_prop_string (ctx, -2, "getPosition");
            duk_push_c_function (ctx, native_transport_setPosition, 1);
            duk_put_prop_string (ctx, -2, "setPosition");
            duk_push_c_function (ctx, native_transport_getBpm,      0);
            duk_put_prop_string (ctx, -2, "getBpm");
            duk_push_c_function (ctx, native_transport_setBpm,      1);
            duk_put_prop_string (ctx, -2, "setBpm");
            duk_push_c_function (ctx, native_transport_isRecording,   0);
            duk_put_prop_string (ctx, -2, "isRecording");
            duk_push_c_function (ctx, native_transport_setRecording,  1);
            duk_put_prop_string (ctx, -2, "setRecording");
            duk_push_c_function (ctx, native_transport_getMetronome,  0);
            duk_put_prop_string (ctx, -2, "getMetronome");
            duk_push_c_function (ctx, native_transport_setMetronome,  1);
            duk_put_prop_string (ctx, -2, "setMetronome");
            duk_put_prop_string (ctx, -2, "transport");

            // --- tracks -------------------------------------------------
            duk_push_object (ctx);
            duk_push_c_function (ctx, native_tracks_count,        0);
            duk_put_prop_string (ctx, -2, "count");
            duk_push_c_function (ctx, native_tracks_list,         0);
            duk_put_prop_string (ctx, -2, "list");
            duk_push_c_function (ctx, native_tracks_getName,      1);
            duk_put_prop_string (ctx, -2, "getName");
            duk_push_c_function (ctx, native_tracks_getType,      1);
            duk_put_prop_string (ctx, -2, "getType");
            duk_push_c_function (ctx, native_tracks_isMute,       1);
            duk_put_prop_string (ctx, -2, "isMute");
            duk_push_c_function (ctx, native_tracks_isSolo,       1);
            duk_put_prop_string (ctx, -2, "isSolo");
            duk_push_c_function (ctx, native_tracks_getGain,      1);
            duk_put_prop_string (ctx, -2, "getGain");
            duk_push_c_function (ctx, native_tracks_getVolume,    1);
            duk_put_prop_string (ctx, -2, "getVolume");
            duk_push_c_function (ctx, native_tracks_getPan,       1);
            duk_put_prop_string (ctx, -2, "getPan");
            duk_push_c_function (ctx, native_tracks_isSelected,   1);
            duk_put_prop_string (ctx, -2, "isSelected");
            duk_push_c_function (ctx, native_tracks_regionCount,  1);
            duk_put_prop_string (ctx, -2, "regionCount");
            duk_push_c_function (ctx, native_tracks_isRecording,  1);
            duk_put_prop_string (ctx, -2, "isRecording");
            duk_push_c_function (ctx, native_tracks_setRecording, 2);
            duk_put_prop_string (ctx, -2, "setRecording");
            duk_put_prop_string (ctx, -2, "tracks");

            // --- regions ------------------------------------------------
            duk_push_object (ctx);
            duk_push_c_function (ctx, native_regions_getName,     2);
            duk_put_prop_string (ctx, -2, "getName");
            duk_push_c_function (ctx, native_regions_getStart,    2);
            duk_put_prop_string (ctx, -2, "getStart");
            duk_push_c_function (ctx, native_regions_getLength,   2);
            duk_put_prop_string (ctx, -2, "getLength");
            duk_push_c_function (ctx, native_regions_isSelected,  2);
            duk_put_prop_string (ctx, -2, "isSelected");
            duk_push_c_function (ctx, native_regions_isLoop,      2);
            duk_put_prop_string (ctx, -2, "isLoop");
            duk_push_c_function (ctx, native_regions_getGain,     2);
            duk_put_prop_string (ctx, -2, "getGain");
            duk_push_c_function (ctx, native_regions_noteCount,   2);
            duk_put_prop_string (ctx, -2, "noteCount");
            duk_push_c_function (ctx, native_regions_getNote,     3);
            duk_put_prop_string (ctx, -2, "getNote");
            duk_put_prop_string (ctx, -2, "regions");

            // --- actions ------------------------------------------------
            duk_push_object (ctx);
            duk_push_c_function (ctx, native_actions_moveRegion,    3);
            duk_put_prop_string (ctx, -2, "moveRegion");
            duk_push_c_function (ctx, native_actions_addAudioTrack, 0);
            duk_put_prop_string (ctx, -2, "addAudioTrack");
            duk_push_c_function (ctx, native_actions_addMidiTrack,  0);
            duk_put_prop_string (ctx, -2, "addMidiTrack");
            duk_push_c_function (ctx, native_actions_addInstrumentTrack, 1);
            duk_put_prop_string (ctx, -2, "addInstrumentTrack");
            duk_push_c_function (ctx, native_actions_deleteTrack,    1);
            duk_put_prop_string (ctx, -2, "deleteTrack");
            duk_push_c_function (ctx, native_actions_setRegionGain,  3);
            duk_put_prop_string (ctx, -2, "setRegionGain");
            duk_push_c_function (ctx, native_actions_addMidiClip,    3);
            duk_put_prop_string (ctx, -2, "addMidiClip");
            duk_push_c_function (ctx, native_actions_addNote,        DUK_VARARGS);
            duk_put_prop_string (ctx, -2, "addNote");
            duk_push_c_function (ctx, native_actions_deleteNote,     3);
            duk_put_prop_string (ctx, -2, "deleteNote");
            duk_push_c_function (ctx, native_actions_undo,           0);
            duk_put_prop_string (ctx, -2, "undo");
            duk_push_c_function (ctx, native_actions_redo,           0);
            duk_put_prop_string (ctx, -2, "redo");
            duk_put_prop_string (ctx, -2, "actions");

            // --- plugins ------------------------------------------------
            duk_push_object (ctx);
            duk_push_c_function (ctx, native_plugins_list, 0);
            duk_put_prop_string (ctx, -2, "list");
            duk_put_prop_string (ctx, -2, "plugins");

            // --- ui (non-undoable side-effects) -------------------------
            duk_push_object (ctx);
            duk_push_c_function (ctx, native_ui_openPlugin,    1);
            duk_put_prop_string (ctx, -2, "openPlugin");
            duk_push_c_function (ctx, native_ui_openTrackView, 0);
            duk_put_prop_string (ctx, -2, "openTrackView");
            duk_put_prop_string (ctx, -2, "ui");

            // --- project (save / load / clear) --------------------------
            duk_push_object (ctx);
            duk_push_c_function (ctx, native_project_save,           1);
            duk_put_prop_string (ctx, -2, "save");
            duk_push_c_function (ctx, native_project_load,           1);
            duk_put_prop_string (ctx, -2, "load");
            duk_push_c_function (ctx, native_project_clear,          0);
            duk_put_prop_string (ctx, -2, "clear");
            duk_push_c_function (ctx, native_project_getCurrentPath, 0);
            duk_put_prop_string (ctx, -2, "getCurrentPath");
            duk_push_c_function (ctx, native_project_isDirty,        0);
            duk_put_prop_string (ctx, -2, "isDirty");
            duk_put_prop_string (ctx, -2, "project");

            // --- test (assertion helpers for self-test macros) ----------
            duk_push_object (ctx);
            duk_push_c_function (ctx, native_test_assert,        2);
            duk_put_prop_string (ctx, -2, "assert");
            duk_push_c_function (ctx, native_test_assertEquals,  3);
            duk_put_prop_string (ctx, -2, "assertEquals");
            duk_push_c_function (ctx, native_test_assertThrows,  2);
            duk_put_prop_string (ctx, -2, "assertThrows");
            duk_put_prop_string (ctx, -2, "test");
        }
        duk_put_global_string (ctx, "synthlab");
    }
}

// ============================================================================
// Autocomplete: schema + popup + CodeEditor subclass
// ============================================================================
//
// The schema mirrors registerSynthlabApi above by hand - single source of truth
// would be nicer, but the duktape registration is data-driven by C function
// pointers and the popup needs human-readable arg names, so keeping the two in
// sync manually is the lesser evil. Whenever a native is added/removed up
// there, update the table below.

struct ApiMember
{
    juce::String name;       // matched against the prefix the user is typing
    juce::String insertion;  // what gets inserted on accept (e.g. "play()")
    juce::String hint;       // shown right-aligned, dim
    // -1 = not a function (namespace member): insert as-is, caret at end.
    //  0 = function with no args: insert "name()", caret AFTER the ')'.
    // >0 = function with args   : insert "name()", caret BETWEEN the parens.
    int argCount;
};

namespace
{
    bool isIdentChar (juce_wchar c)
    {
        return juce::CharacterFunctions::isLetterOrDigit (c) || c == '_' || c == '$';
    }

    const std::vector<ApiMember>& membersForPath (const juce::String& path)
    {
        // Built once on first call. The keys mirror the dotted paths that
        // registerSynthlabApi puts on the global object.
        static const std::map<juce::String, std::vector<ApiMember>> table = {
            { "", {
                { "synthlab", "synthlab", "Synthlab API root",                  -1 },
                { "console",  "console",  "Logging",                            -1 },
                { "print",    "print()",  "(...args) -> output",                  1 },
                { "alert",    "alert()",  "(message, [title]) -> modal info dialog", 1 },
                { "prompt",   "prompt()", "(msg, [default], [title]) -> string | null", 1 },
            }},
            { "synthlab", {
                { "transport", "transport", "Playhead, BPM",                       -1 },
                { "tracks",    "tracks",    "Read track state",                    -1 },
                { "regions",   "regions",   "Read region state",                   -1 },
                { "actions",   "actions",   "Undoable edits + undo/redo",          -1 },
                { "plugins",   "plugins",   "Available instruments",               -1 },
                { "ui",        "ui",        "UI side-effects (open windows, etc.)",-1 },
                { "project",   "project",   "Save / load / clear the project",     -1 },
                { "test",      "test",      "Assertion helpers for smoke tests",   -1 },
            }},
            { "synthlab.transport", {
                { "play",         "play()",         "Start playback",                 0 },
                { "stop",         "stop()",         "Stop playback",                  0 },
                { "toggle",       "toggle()",       "Toggle play/stop",               0 },
                { "isPlaying",    "isPlaying()",    "-> bool",                         0 },
                { "getPosition",  "getPosition()",  "-> seconds",                      0 },
                { "setPosition",  "setPosition()",  "(seconds)",                      1 },
                { "getBpm",       "getBpm()",       "-> tempo",                        0 },
                { "setBpm",       "setBpm()",       "(value)",                        1 },
                { "isRecording",  "isRecording()",  "-> bool - global record state",   0 },
                { "setRecording", "setRecording()", "(bool) - start/stop recording",  1 },
                { "getMetronome", "getMetronome()", "-> bool",                         0 },
                { "setMetronome", "setMetronome()", "(bool) - click during record",   1 },
            }},
            { "synthlab.tracks", {
                { "count",       "count()",       "-> int",                            0 },
                { "list",        "list()",        "-> string[] names",                 0 },
                { "getName",     "getName()",     "(i) ->string",                     1 },
                { "getType",     "getType()",     "(i) -> audio|midi|instrument",      1 },
                { "isMute",      "isMute()",      "(i) -> bool",                       1 },
                { "isSolo",      "isSolo()",      "(i) -> bool",                       1 },
                { "getGain",     "getGain()",     "(i) -> number",                     1 },
                { "getVolume",   "getVolume()",   "(i) -> number",                     1 },
                { "getPan",      "getPan()",      "(i) -> number",                     1 },
                { "isSelected",  "isSelected()",  "(i) -> bool",                       1 },
                { "regionCount", "regionCount()", "(i) -> int",                        1 },
                { "isRecording", "isRecording()", "(i) -> bool - track arm",           1 },
                { "setRecording","setRecording()","(i, -> bool) - arm/disarm track",     1 },
            }},
            { "synthlab.regions", {
                { "getName",    "getName()",    "(t, r) -> string",          1 },
                { "getStart",   "getStart()",   "(t, r) -> seconds",         1 },
                { "getLength",  "getLength()", "(t, r) -> seconds",          1 },
                { "isSelected", "isSelected()", "(t, r) -> bool",            1 },
                { "isLoop",     "isLoop()",     "(t, r) -> bool",            1 },
                { "getGain",    "getGain()",    "(t, r) -> number (1.0 = unity)", 1 },
                { "noteCount",  "noteCount()",  "(t, r) -> int - MIDI note-on count", 1 },
                { "getNote",    "getNote()",    "(t, r, i) -> {pitch,velocity,channel,startSec,lengthSec}", 1 },
            }},
            { "synthlab.actions", {
                { "moveRegion",         "moveRegion()",         "(t, r, deltaSeconds) -> bool",       1 },
                { "setRegionGain",      "setRegionGain()",      "(t, r, gain) -> bool",                1 },
                { "addMidiClip",        "addMidiClip()",        "(t, startSec, lenSec) -> idx (no undo)",1 },
                { "addNote",            "addNote()",            "(t, r, pitch, vel, startSec, lenSec, [ch]) -> bool", 1 },
                { "deleteNote",         "deleteNote()",         "(t, r, noteIdx) -> bool",            1 },
                { "addAudioTrack",      "addAudioTrack()",      "-> new track index",                  0 },
                { "addMidiTrack",       "addMidiTrack()",       "-> new track index",                  0 },
                { "addInstrumentTrack", "addInstrumentTrack()", "(pluginName) -> new index",           1 },
                { "deleteTrack",        "deleteTrack()",        "(t) -> bool",                          1 },
                { "undo",               "undo()",               "-> bool - pops one UndoManager step",  0 },
                { "redo",               "redo()",               "-> bool",                              0 },
            }},
            { "console", {
                { "log",   "log()",   "(...args) -> output",       1 },
                { "error", "error()", "(...args) -> error output", 1 },
            }},
            { "synthlab.plugins", {
                { "list",  "list()",  "-> string[] instrument names", 0 },
            }},
            { "synthlab.ui", {
                { "openPlugin",    "openPlugin()",    "(trackIdx) -> bool - opens VST editor", 1 },
                { "openTrackView", "openTrackView()", "-> bool - bring Tracks tab forward",     0 },
            }},
            { "synthlab.project", {
                { "save",           "save()",           "(path) -> bool - write .slb (no dialog)", 1 },
                { "load",           "load()",           "(path) -> bool - load .slb (no prompt)",  1 },
                { "clear",          "clear()",          "-> bool - File > New, no prompt",         0 },
                { "getCurrentPath", "getCurrentPath()", "-> string - current .slb path",           0 },
                { "isDirty",        "isDirty()",        "-> bool",                                  0 },
            }},
            { "synthlab.test", {
                { "assert",        "assert()",        "(cond, [msg]) - throws if !cond",       1 },
                { "assertEquals",  "assertEquals()",  "(actual, expected, [msg])",             1 },
                { "assertThrows",  "assertThrows()",  "(fn, [msg]) - expects fn() to throw",   1 },
            }},
        };
        static const std::vector<ApiMember> empty;
        auto it = table.find (path);
        return it != table.end() ? it->second : empty;
    }
}  // anonymous namespace - classes follow at file scope so they match the
   // forward declarations in JavaScriptEditorComponent.h (forward-declaring
   // an anonymous-namespace type at global scope mismatches at link time).

    // ------------------------------------------------------------------------
    // Popup component: a thin ListBox wrapped in a panel, positioned at the
    // caret. Does NOT take keyboard focus - all navigation keys come through
    // the editor's keyPressed override, the popup is purely visual.

    class AutoCompletePopup : public juce::Component,
                              public juce::ListBoxModel
    {
    public:
        AutoCompletePopup()
        {
            list.setModel (this);
            list.setRowHeight (rowHeight);
            list.setWantsKeyboardFocus (false);
            list.setColour (juce::ListBox::backgroundColourId, juce::Colour (0xff1e1e1e));
            list.setColour (juce::ListBox::outlineColourId, juce::Colour (0xff3a3a3a));
            list.setOutlineThickness (1);
            addAndMakeVisible (list);

            setWantsKeyboardFocus (false);
            setVisible (false);
        }

        bool isOpen() const { return open; }

        void showFor (const juce::String& path,
                      const juce::String& prefix,
                      juce::Point<int> topLeftInParent)
        {
            source = membersForPath (path);
            currentPrefix = prefix;
            applyFilter();
            if (filtered.empty()) { hide(); return; }

            list.selectRow (0);
            list.scrollToEnsureRowIsOnscreen (0);

            const int h = juce::jmin ((int) filtered.size(), maxVisibleRows) * rowHeight + 2;
            setSize (popupWidth, h);

            // Flip above caret if we'd run off the bottom of the parent.
            if (auto* parent = getParentComponent())
            {
                int y = topLeftInParent.y;
                if (y + h > parent->getHeight() - 4)
                    y = topLeftInParent.y - h - rowHeight;  // above the caret line
                int x = juce::jlimit (4, parent->getWidth() - popupWidth - 4,
                                      topLeftInParent.x);
                setTopLeftPosition (x, juce::jmax (4, y));
            }
            else
            {
                setTopLeftPosition (topLeftInParent);
            }

            setVisible (true);
            toFront (false);
            open = true;
        }

        void updatePrefix (const juce::String& newPrefix)
        {
            currentPrefix = newPrefix;
            applyFilter();
            if (filtered.empty()) { hide(); return; }
            list.updateContent();
            if (list.getSelectedRow() < 0 || list.getSelectedRow() >= (int) filtered.size())
                list.selectRow (0);
            list.scrollToEnsureRowIsOnscreen (list.getSelectedRow());
            repaint();
        }

        void hide()
        {
            if (! open) return;
            open = false;
            setVisible (false);
        }

        void moveSelection (int delta)
        {
            if (filtered.empty()) return;
            int row = list.getSelectedRow();
            if (row < 0) row = 0;
            row = juce::jlimit (0, (int) filtered.size() - 1, row + delta);
            list.selectRow (row);
            list.scrollToEnsureRowIsOnscreen (row);
        }

        const ApiMember* selectedMember() const
        {
            const int row = list.getSelectedRow();
            if (row < 0 || row >= (int) filtered.size()) return nullptr;
            return &filtered[(size_t) row];
        }

        // Callback for the editor - runs on the message thread.
        std::function<void(const ApiMember&)> onAccept;

        // -- Component --------------------------------------------------------
        void resized() override        { list.setBounds (getLocalBounds()); }
        void paint (juce::Graphics&) override {}

        // -- ListBoxModel -----------------------------------------------------
        int getNumRows() override { return (int) filtered.size(); }

        void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool sel) override
        {
            if (row < 0 || row >= (int) filtered.size()) return;
            const auto& m = filtered[(size_t) row];

            g.fillAll (sel ? juce::Colour (0xff3a3a55) : juce::Colour (0xff1e1e1e));

            // Insertion text on the left
            g.setColour (sel ? juce::Colours::white : juce::Colour (0xffe0e0e0));
            g.setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(), 13.0f,
                                   juce::Font::plain));
            g.drawText (m.insertion, 6, 0, w - 12, h, juce::Justification::centredLeft);

            // Hint text on the right
            const int hintWidth = juce::jmin (w / 2, 200);
            g.setColour (sel ? juce::Colour (0xffd0d0e0) : juce::Colour (0xff8a8a8a));
            g.setFont (juce::Font (12.0f, juce::Font::plain));
            g.drawText (m.hint, w - hintWidth - 6, 0, hintWidth, h,
                        juce::Justification::centredRight);
        }

        void listBoxItemClicked (int row, const juce::MouseEvent&) override
        {
            if (row < 0 || row >= (int) filtered.size()) return;
            list.selectRow (row);
            if (onAccept) onAccept (filtered[(size_t) row]);
        }

    private:
        void applyFilter()
        {
            filtered.clear();
            if (currentPrefix.isEmpty())
            {
                filtered = source;
            }
            else
            {
                const auto needle = currentPrefix.toLowerCase();
                for (const auto& m : source)
                    if (m.name.toLowerCase().startsWith (needle))
                        filtered.push_back (m);
            }
            list.updateContent();
        }

        juce::ListBox list { "synthlab.autocomplete", nullptr };
        std::vector<ApiMember> source;
        std::vector<ApiMember> filtered;
        juce::String currentPrefix;
        bool open = false;

        static constexpr int rowHeight     = 22;
        static constexpr int maxVisibleRows = 8;
        static constexpr int popupWidth     = 360;
    };

    // ------------------------------------------------------------------------
    // Code editor subclass: hooks keyPressed for popup navigation and listens
    // on the CodeDocument for text changes so the popup can react to typing,
    // pasting, and backspace without us having to track each key individually.

    class JavaScriptCodeEditor : public juce::CodeEditorComponent,
                                 public juce::CodeDocument::Listener
    {
    public:
        JavaScriptCodeEditor (juce::CodeDocument& doc, juce::CodeTokeniser* tok)
            : juce::CodeEditorComponent (doc, tok), doc (doc)
        {
            doc.addListener (this);
        }

        ~JavaScriptCodeEditor() override
        {
            doc.removeListener (this);
        }

        // Wired by JavaScriptEditorComponent after both objects exist.
        void setPopup (AutoCompletePopup* p)
        {
            popup = p;
            if (popup != nullptr)
            {
                popup->onAccept = [this](const ApiMember& m) { acceptMember (m); };
            }
        }

        // -- CodeEditorComponent --------------------------------------------
        bool keyPressed (const juce::KeyPress& k) override
        {
            // When the popup is visible we steal navigation keys so they
            // drive selection instead of caret/newline.
            if (popup != nullptr && popup->isOpen())
            {
                if (k.isKeyCode (juce::KeyPress::escapeKey)) { popup->hide(); return true; }
                if (k.isKeyCode (juce::KeyPress::upKey))     { popup->moveSelection (-1); return true; }
                if (k.isKeyCode (juce::KeyPress::downKey))   { popup->moveSelection (+1); return true; }
                if (k.isKeyCode (juce::KeyPress::returnKey)
                 || k.isKeyCode (juce::KeyPress::tabKey))
                {
                    if (auto* m = popup->selectedMember())
                    {
                        acceptMember (*m);
                        return true;
                    }
                }
            }

            // Autocomplete trigger. Zwei Tastaturkombos, weil Ctrl-Space auf
            // macOS standardmaessig vom System abgefangen wird (System-
            // Settings -> Keyboard -> Keyboard Shortcuts -> Input Sources ->
            // "Select the previous input source"). Wenn der User diesen
            // System-Shortcut deaktiviert, funktioniert Ctrl-Space; sonst
            // bleibt Cmd-I als kollisionsfreier Fallback. NICHT Cmd-Space
            // (das ist Spotlight).
            if ((k.getKeyCode() == juce::KeyPress::spaceKey
                 && k.getModifiers().isCtrlDown())
             || (k.getModifiers().isCommandDown()
                 && (k.getKeyCode() == 'I' || k.getKeyCode() == 'i')))
            {
                triggerPopup();
                return true;
            }

            // Cmd-R / Ctrl-R or Cmd-Enter / Ctrl-Enter: run the buffer. Same
            // semantics as the Run button - walks up to the host container so
            // we don't have to plumb a callback through. Placed AFTER the
            // popup-open branch so Enter still accepts a suggestion when the
            // popup is showing.
            const bool mod = k.getModifiers().isCommandDown() || k.getModifiers().isCtrlDown();
            if (mod && (k.getKeyCode() == 'R' || k.getKeyCode() == 'r'
                     || k.isKeyCode (juce::KeyPress::returnKey)))
            {
                if (auto* host = findParentComponentOfClass<JavaScriptEditorComponent>())
                {
                    host->runScript();
                    return true;
                }
            }

            return juce::CodeEditorComponent::keyPressed (k);
        }

        // -- CodeDocument::Listener -----------------------------------------
        //
        // Trigger policy: popup opens only on '.' or Cmd/Ctrl-Space, never on
        // a bare letter - that policy keeps the popup unobtrusive during
        // normal typing (writing variable names, JS keywords, etc.). Once
        // open, every keystroke filters or dismisses it.
        void codeDocumentTextInserted (const juce::String& text, int /*pos*/) override
        {
            if (insertingFromCompletion) return;

            // Big insertions (paste, file load, programmatic replace) hide
            // any open popup and never trigger a new one.
            if (text.length() > 2)
            {
                if (popup != nullptr && popup->isOpen())
                    popup->hide();
                return;
            }

            if (popup != nullptr && popup->isOpen())
            {
                refreshPopup();
                return;
            }

            if (text == ".")
                triggerPopupFromDot();
        }

        void codeDocumentTextDeleted (int /*start*/, int /*end*/) override
        {
            if (insertingFromCompletion) return;
            if (popup != nullptr && popup->isOpen())
                refreshPopup();
        }

    private:
        // Walks backwards from the caret to extract the dotted path before
        // the most recent identifier ("synthlab.transport") and the prefix
        // currently being typed ("pla"). Both can be empty.
        void extractContext (juce::String& outPath, juce::String& outPrefix) const
        {
            const auto caret = getCaretPos();
            const auto line  = caret.getLineText();
            const int col    = caret.getIndexInLine();

            int i = col;
            while (i > 0 && isIdentChar (line[i - 1])) --i;
            outPrefix = line.substring (i, col);

            if (i > 0 && line[i - 1] == '.')
            {
                int k = i - 1;        // start scanning at the dot
                int pathEnd = k;
                while (k > 0)
                {
                    int idEnd = k;
                    while (k > 0 && isIdentChar (line[k - 1])) --k;
                    if (k == idEnd) break;            // dot with no identifier in front
                    if (k > 0 && line[k - 1] == '.') { --k; continue; }
                    break;
                }
                outPath = line.substring (k, pathEnd);
            }
            else
            {
                outPath = juce::String();
            }
        }

        // Explicit trigger (Cmd/Ctrl-Space): allow an empty path so the user
        // can pull up the top-level globals (synthlab / console / print) at
        // any caret position.
        void triggerPopup()
        {
            juce::String path, prefix;
            extractContext (path, prefix);

            const auto& members = membersForPath (path);
            if (members.empty()) { if (popup) popup->hide(); return; }

            openPopupAtCaret (path, prefix);
        }

        // Auto-trigger after the user typed '.': require a non-empty path so
        // a bare '.' (after whitespace, semicolon, etc.) doesn't pop the
        // globals menu. `1.` and `foo.` (unknown identifier) also no-op
        // because membersForPath returns empty.
        void triggerPopupFromDot()
        {
            juce::String path, prefix;
            extractContext (path, prefix);
            if (path.isEmpty()) return;

            const auto& members = membersForPath (path);
            if (members.empty()) { if (popup) popup->hide(); return; }

            openPopupAtCaret (path, prefix);
        }

        void refreshPopup()
        {
            if (popup == nullptr) { return; }
            juce::String path, prefix;
            extractContext (path, prefix);

            // If the path no longer matches a known namespace, drop the popup.
            const auto& members = membersForPath (path);
            if (members.empty()) { popup->hide(); return; }

            // If we crossed a path boundary (e.g. dot got deleted), reopen
            // with the new member set instead of just filtering.
            popup->showFor (path, prefix, popupAnchorInParent());
        }

        void openPopupAtCaret (const juce::String& path, const juce::String& prefix)
        {
            if (popup == nullptr) return;
            popup->showFor (path, prefix, popupAnchorInParent());
        }

        juce::Point<int> popupAnchorInParent() const
        {
            auto caretRect = getCharacterBounds (getCaretPos());
            // Translate from editor-local to parent (JavaScriptEditorComponent)
            // coords. Both share the same parent.
            return getPosition() + caretRect.getBottomLeft();
        }

        void acceptMember (const ApiMember& m)
        {
            if (popup != nullptr) popup->hide();

            // Replace the prefix under the caret with the member's insertion.
            const auto caret = getCaretPos();
            const auto line  = caret.getLineText();
            const int col    = caret.getIndexInLine();
            int i = col;
            while (i > 0 && isIdentChar (line[i - 1])) --i;
            const int prefixLen = col - i;

            const juce::CodeDocument::Position selStart (doc, caret.getPosition() - prefixLen);
            selectRegion (selStart, caret);

            insertingFromCompletion = true;
            insertTextAtCaret (m.insertion);
            insertingFromCompletion = false;

            // Functions with args: drop the caret inside the parens so the
            // user can start typing the first argument immediately.
            if (m.argCount > 0)
            {
                const juce::CodeDocument::Position after (doc,
                    getCaretPos().getPosition() - 1);
                moveCaretTo (after, false);
            }

            grabKeyboardFocus();
        }

        juce::CodeDocument& doc;
        AutoCompletePopup* popup = nullptr;
        bool insertingFromCompletion = false;
    };

JavaScriptEditorComponent::JavaScriptEditorComponent()
{
    titleLabel.setText ("Untitled.js", dontSendNotification);
    titleLabel.setColour (Label::textColourId, Colours::lightgrey);
    titleLabel.setFont (Font (14.0f, Font::bold));
    addAndMakeVisible (titleLabel);

    addAndMakeVisible (newButton);
    addAndMakeVisible (openButton);
    addAndMakeVisible (saveButton);
    addAndMakeVisible (saveAsButton);
    addAndMakeVisible (runButton);
    addAndMakeVisible (snippetsButton);
    addAndMakeVisible (clearButton);

    newButton     .addListener (this);
    openButton    .addListener (this);
    saveButton    .addListener (this);
    saveAsButton  .addListener (this);
    runButton     .addListener (this);
    snippetsButton.addListener (this);
    clearButton   .addListener (this);

    editor = std::make_unique<JavaScriptCodeEditor> (document, &tokeniser);
    editor->setColourScheme (tokeniser.getDefaultColourScheme());
    editor->setColour (CodeEditorComponent::backgroundColourId, Colour (0xff1e1e1e));
    editor->setColour (CodeEditorComponent::defaultTextColourId, Colours::white);
    editor->setColour (CodeEditorComponent::lineNumberBackgroundId, Colour (0xff2a2a2a));
    editor->setColour (CodeEditorComponent::lineNumberTextId, Colour (0xff7a7a7a));
    editor->setColour (CodeEditorComponent::highlightColourId, Colour (0xff3a3a55));
    editor->setFont (Font (Font::getDefaultMonospacedFontName(), 14.0f, Font::plain));
    addAndMakeVisible (editor.get());

    // Popup floats above the editor - added AFTER the editor so it paints on
    // top by default. Initially hidden; opened by the editor when context
    // matches the synthlab.* schema. The editor calls toFront() on show, but
    // child ordering also matters for paint/hit-test.
    autoComplete = std::make_unique<AutoCompletePopup>();
    addChildComponent (autoComplete.get());
    editor->setPopup (autoComplete.get());

    output.setMultiLine (true);
    output.setReadOnly (true);
    output.setCaretVisible (false);
    output.setScrollbarsShown (true);
    output.setColour (TextEditor::backgroundColourId, Colour (0xff141414));
    output.setColour (TextEditor::textColourId, Colours::lightgrey);
    output.setColour (TextEditor::outlineColourId, Colour (0xff333333));
    output.setFont (Font (Font::getDefaultMonospacedFontName(), 13.0f, Font::plain));
    addAndMakeVisible (output);

    document.replaceAllContent (
        "// Synthlab JavaScript editor (Duktape)\n"
        "// console.log() and print() write to the output panel below.\n"
        "// alert(msg) / prompt(msg, default) show modal dialogs.\n"
        "// Run with Cmd-R / Cmd-Enter (Ctrl on Win/Linux), or the Run button.\n"
        "// Autocomplete: Ctrl-Space (requires disabling the macOS input-\n"
        "// source shortcut) or Cmd-I as a kollisionsfreier fallback.\n"
        "//\n"
        "// Available API:\n"
        "//   synthlab.transport.play() / stop() / toggle()\n"
        "//   synthlab.transport.isPlaying() / getPosition() / setPosition(s)\n"
        "//   synthlab.transport.getBpm() / setBpm(value)\n"
        "//\n"
        "//   synthlab.tracks.count() / list()\n"
        "//   synthlab.tracks.getName(i) / getType(i) / isMute(i) / isSolo(i)\n"
        "//   synthlab.tracks.getGain(i) / getVolume(i) / getPan(i)\n"
        "//   synthlab.tracks.isSelected(i) / regionCount(i)\n"
        "//\n"
        "//   synthlab.regions.getName(t, r) / getStart(t, r) / getLength(t, r)\n"
        "//   synthlab.regions.isSelected(t, r) / isLoop(t, r) / getGain(t, r)\n"
        "//\n"
        "//   synthlab.actions.moveRegion(t, r, deltaSeconds)\n"
        "//   synthlab.actions.setRegionGain(t, r, gain)  // 1.0 = unity\n"
        "//   synthlab.actions.addAudioTrack() / addMidiTrack() -> new index\n"
        "//   synthlab.actions.deleteTrack(t)\n"
        "// (Cmd-Z undoes any of the above.)\n\n"
        "console.log(\"BPM:\", synthlab.transport.getBpm(),\n"
        "            \"Position:\", synthlab.transport.getPosition().toFixed(3), \"s\");\n\n"
        "var n = synthlab.tracks.count();\n"
        "console.log(\"Tracks:\", n);\n"
        "for (var i = 0; i < n; i++) {\n"
        "    var rc = synthlab.tracks.regionCount(i);\n"
        "    console.log(\"  [\" + i + \"]\",\n"
        "                synthlab.tracks.getName(i),\n"
        "                \"(\" + synthlab.tracks.getType(i) + \")\",\n"
        "                \"gain=\" + synthlab.tracks.getGain(i).toFixed(2),\n"
        "                rc + \" region\" + (rc === 1 ? \"\" : \"s\"));\n"
        "    for (var r = 0; r < rc; r++) {\n"
        "        console.log(\"      \" + synthlab.regions.getName(i, r),\n"
        "                    \"@\" + synthlab.regions.getStart(i, r).toFixed(3) + \"s\",\n"
        "                    \"len=\" + synthlab.regions.getLength(i, r).toFixed(3) + \"s\");\n"
        "    }\n"
        "}\n");
}

JavaScriptEditorComponent::~JavaScriptEditorComponent()
{
    newButton     .removeListener (this);
    openButton    .removeListener (this);
    saveButton    .removeListener (this);
    saveAsButton  .removeListener (this);
    runButton     .removeListener (this);
    snippetsButton.removeListener (this);
    clearButton   .removeListener (this);
}

void JavaScriptEditorComponent::paint (Graphics& g)
{
    g.fillAll (Colour (0xff1b1b25));
}

void JavaScriptEditorComponent::resized()
{
    auto area = getLocalBounds();

    auto toolbar = area.removeFromTop (30);
    toolbar.reduce (4, 4);

    const int buttonWidth = 70;
    newButton   .setBounds (toolbar.removeFromLeft (buttonWidth));
    toolbar.removeFromLeft (4);
    openButton  .setBounds (toolbar.removeFromLeft (buttonWidth));
    toolbar.removeFromLeft (4);
    saveButton  .setBounds (toolbar.removeFromLeft (buttonWidth));
    toolbar.removeFromLeft (4);
    saveAsButton.setBounds (toolbar.removeFromLeft (buttonWidth));
    toolbar.removeFromLeft (12);
    runButton   .setBounds (toolbar.removeFromLeft (buttonWidth));
    toolbar.removeFromLeft (12);
    snippetsButton.setBounds (toolbar.removeFromLeft (buttonWidth + 10));
    toolbar.removeFromLeft (12);
    clearButton .setBounds (toolbar.removeFromLeft (buttonWidth + 30));
    toolbar.removeFromLeft (12);
    titleLabel  .setBounds (toolbar);

    const int outputHeight = jmax (80, area.getHeight() / 4);
    auto outputArea = area.removeFromBottom (outputHeight);
    outputArea.reduce (4, 4);
    output.setBounds (outputArea);

    area.reduce (4, 4);
    if (editor != nullptr)
        editor->setBounds (area);

    // Popup positions itself at the caret on show - close it if the layout
    // shifts under it so the user doesn't see a stale floating menu.
    if (autoComplete != nullptr && autoComplete->isOpen())
        autoComplete->hide();
}

void JavaScriptEditorComponent::buttonClicked (Button* b)
{
    if (b == &newButton)             newFile();
    else if (b == &openButton)       openFile();
    else if (b == &saveButton)       saveFile();
    else if (b == &saveAsButton)     saveFileAs();
    else if (b == &runButton)        runScript();
    else if (b == &snippetsButton)   showSnippetsMenu();
    else if (b == &clearButton)      output.clear();
}

void JavaScriptEditorComponent::showSnippetsMenu()
{
    // Capture `this` via a SafePointer so a teardown mid-popup (e.g. tab
    // switch while the menu is open) cannot dereference a stale editor.
    Component::SafePointer<JavaScriptEditorComponent> safeThis (this);

    auto insert = [safeThis](const String& text)
    {
        if (auto* self = safeThis.getComponent())
        {
            if (self->editor != nullptr)
            {
                self->editor->insertTextAtCaret (text);
                self->editor->grabKeyboardFocus();
            }
        }
    };

    PopupMenu menu;

    // ── Transport ────────────────────────────────────────────────────────
    PopupMenu transport;
    transport.addItem ("Play",                  [insert]() { insert ("synthlab.transport.play();\n"); });
    transport.addItem ("Stop",                  [insert]() { insert ("synthlab.transport.stop();\n"); });
    transport.addItem ("Toggle Play/Stop",      [insert]() { insert ("synthlab.transport.toggle();\n"); });
    transport.addItem ("Is Playing?",           [insert]() { insert ("console.log(\"Playing:\", synthlab.transport.isPlaying());\n"); });
    transport.addItem ("Get Position",          [insert]() {
        insert ("var pos = synthlab.transport.getPosition();\n"
                "console.log(\"Position:\", pos.toFixed(3), \"s\");\n");
    });
    transport.addItem ("Jump to Start",         [insert]() { insert ("synthlab.transport.setPosition(0);\n"); });
    transport.addItem ("Get BPM",               [insert]() {
        insert ("console.log(\"BPM:\", synthlab.transport.getBpm());\n");
    });
    transport.addItem ("Set BPM (120)",         [insert]() { insert ("synthlab.transport.setBpm(120);\n"); });
    menu.addSubMenu ("Transport", transport);

    // ── Tracks ───────────────────────────────────────────────────────────
    PopupMenu tracks;
    tracks.addItem ("Track Count",              [insert]() {
        insert ("console.log(\"Tracks:\", synthlab.tracks.count());\n");
    });
    tracks.addItem ("List Track Names",         [insert]() {
        insert ("synthlab.tracks.list().forEach(function(n) { console.log(n); });\n");
    });
    tracks.addItem ("Iterate Tracks (loop)",    [insert]() {
        insert ("for (var i = 0; i < synthlab.tracks.count(); i++) {\n"
                "    console.log(i, synthlab.tracks.getName(i), synthlab.tracks.getType(i));\n"
                "}\n");
    });
    tracks.addItem ("Track Info Dump",          [insert]() {
        insert ("var i = 0;\n"
                "console.log(synthlab.tracks.getName(i),\n"
                "    \"type=\" + synthlab.tracks.getType(i),\n"
                "    \"gain=\" + synthlab.tracks.getGain(i).toFixed(2),\n"
                "    \"vol=\"  + synthlab.tracks.getVolume(i).toFixed(2),\n"
                "    \"pan=\"  + synthlab.tracks.getPan(i).toFixed(2),\n"
                "    synthlab.tracks.isMute(i) ? \"MUTE\" : \"\",\n"
                "    synthlab.tracks.isSolo(i) ? \"SOLO\" : \"\");\n");
    });
    menu.addSubMenu ("Tracks", tracks);

    // ── Regions ──────────────────────────────────────────────────────────
    PopupMenu regions;
    regions.addItem ("Iterate Regions of Track 0", [insert]() {
        insert ("var t = 0;\n"
                "for (var r = 0; r < synthlab.tracks.regionCount(t); r++) {\n"
                "    console.log(r, synthlab.regions.getName(t, r),\n"
                "                \"@\" + synthlab.regions.getStart(t, r).toFixed(3) + \"s\",\n"
                "                \"len=\" + synthlab.regions.getLength(t, r).toFixed(3),\n"
                "                \"gain=\" + synthlab.regions.getGain(t, r).toFixed(2));\n"
                "}\n");
    });
    regions.addItem ("Region Info",             [insert]() {
        insert ("var t = 0, r = 0;\n"
                "console.log(\"Name:\",   synthlab.regions.getName(t, r));\n"
                "console.log(\"Start:\",  synthlab.regions.getStart(t, r).toFixed(3), \"s\");\n"
                "console.log(\"Length:\", synthlab.regions.getLength(t, r).toFixed(3), \"s\");\n"
                "console.log(\"Gain:\",   synthlab.regions.getGain(t, r));\n");
    });
    menu.addSubMenu ("Regions", regions);

    // ── Actions (alle undobar) ───────────────────────────────────────────
    PopupMenu actions;
    actions.addItem ("Move Region (+1 sec)",    [insert]() {
        insert ("synthlab.actions.moveRegion(0, 0, 1.0);  // track, region, deltaSeconds\n");
    });
    actions.addItem ("Set Region Gain",         [insert]() {
        insert ("synthlab.actions.setRegionGain(0, 0, 0.5);  // 1.0 = unity, 0 = mute\n");
    });
    actions.addItem ("Add Audio Track",         [insert]() {
        insert ("var idx = synthlab.actions.addAudioTrack();\n"
                "console.log(\"New audio track at index\", idx);\n");
    });
    actions.addItem ("Add MIDI Track",          [insert]() {
        insert ("var idx = synthlab.actions.addMidiTrack();\n"
                "console.log(\"New MIDI track at index\", idx);\n");
    });
    actions.addItem ("Add Instrument (prompt)", [insert]() {
        insert ("var name = prompt(\"Plugin name?\", \"\");\n"
                "if (name !== null) {\n"
                "    var idx = synthlab.actions.addInstrumentTrack(name);\n"
                "    if (idx < 0) alert(\"Failed: \" + name);\n"
                "    else console.log(\"Added at index\", idx);\n"
                "}\n");
    });
    actions.addItem ("Delete Track 0",          [insert]() {
        insert ("synthlab.actions.deleteTrack(0);  // undoable via Cmd-Z\n");
    });
    menu.addSubMenu ("Actions", actions);

    // ── Plugins ──────────────────────────────────────────────────────────
    PopupMenu plugins;
    plugins.addItem ("List Plugins",            [insert]() {
        insert ("var plugs = synthlab.plugins.list();\n"
                "console.log(\"Available:\", plugs.length);\n"
                "plugs.forEach(function(n) { console.log(\"  \\u2022\", n); });\n");
    });
    plugins.addItem ("Pick Plugin (prompt)",    [insert]() {
        insert ("var plugs = synthlab.plugins.list();\n"
                "console.log(\"Available:\\n  \" + plugs.join(\"\\n  \"));\n"
                "var name = prompt(\"Which plugin?\", plugs[0] || \"\");\n"
                "if (name !== null) synthlab.actions.addInstrumentTrack(name);\n");
    });
    plugins.addItem ("Open Plugin Window for Track 0", [insert]() {
        insert ("synthlab.ui.openPlugin(0);  // opens VST editor for track 0\n");
    });
    menu.addSubMenu ("Plugins", plugins);

    // ── Dialogs / Logging ────────────────────────────────────────────────
    PopupMenu dialogs;
    dialogs.addItem ("alert",                   [insert]() { insert ("alert(\"Hello from Synthlab\");\n"); });
    dialogs.addItem ("prompt",                  [insert]() {
        insert ("var s = prompt(\"Name?\", \"default\");\n"
                "if (s !== null) console.log(\"Got:\", s);\n");
    });
    dialogs.addItem ("console.log",             [insert]() { insert ("console.log(\"...\");\n"); });
    dialogs.addItem ("console.error",           [insert]() { insert ("console.error(\"...\");\n"); });
    dialogs.addItem ("print",                   [insert]() { insert ("print(\"...\");\n"); });
    menu.addSubMenu ("Dialogs / Logging", dialogs);

    // ── Templates (full mini-macros) ─────────────────────────────────────
    PopupMenu templates;
    templates.addItem ("Project summary",       [insert]() {
        insert ("var n = synthlab.tracks.count();\n"
                "console.log(\"=== Project ===\");\n"
                "console.log(\"BPM:\",    synthlab.transport.getBpm());\n"
                "console.log(\"Tracks:\", n);\n"
                "var totalRegions = 0;\n"
                "for (var i = 0; i < n; i++) totalRegions += synthlab.tracks.regionCount(i);\n"
                "console.log(\"Regions:\", totalRegions);\n");
    });
    templates.addItem ("Halve all region gains", [insert]() {
        insert ("// Halves the gain of every audio region - undo with Cmd-Z (one tx).\n"
                "for (var t = 0; t < synthlab.tracks.count(); t++) {\n"
                "    if (synthlab.tracks.getType(t) !== \"audio\") continue;\n"
                "    for (var r = 0; r < synthlab.tracks.regionCount(t); r++) {\n"
                "        var g = synthlab.regions.getGain(t, r);\n"
                "        synthlab.actions.setRegionGain(t, r, g * 0.5);\n"
                "    }\n"
                "}\n"
                "console.log(\"Done.\");\n");
    });
    templates.addItem ("Reset all region gains to 1.0", [insert]() {
        insert ("for (var t = 0; t < synthlab.tracks.count(); t++) {\n"
                "    for (var r = 0; r < synthlab.tracks.regionCount(t); r++) {\n"
                "        synthlab.actions.setRegionGain(t, r, 1.0);\n"
                "    }\n"
                "}\n");
    });
    templates.addItem ("Smoke test: addTrack -> undo roundtrip", [insert]() {
        insert ("// Quick regression check: adding a track and immediately undoing\n"
                "// must leave the project in the original state.\n"
                "var before = synthlab.tracks.count();\n"
                "var idx = synthlab.actions.addAudioTrack();\n"
                "synthlab.test.assert(idx >= 0, \"addAudioTrack should return a valid index\");\n"
                "synthlab.test.assertEquals(synthlab.tracks.count(), before + 1, \"track count after add\");\n"
                "synthlab.actions.undo();\n"
                "synthlab.test.assertEquals(synthlab.tracks.count(), before, \"track count after undo\");\n"
                "console.log(\"PASS: addTrack roundtrip\");\n");
    });
    templates.addItem ("Smoke test: setRegionGain roundtrip", [insert]() {
        insert ("// Mutate a region gain, undo, assert it bounced back to the\n"
                "// original value. Requires at least one audio region in track 0.\n"
                "var t = 0, r = 0;\n"
                "if (synthlab.tracks.regionCount(t) === 0) {\n"
                "    console.error(\"No region on track 0 - load a project with audio first.\");\n"
                "} else {\n"
                "    var before = synthlab.regions.getGain(t, r);\n"
                "    synthlab.actions.setRegionGain(t, r, 0.123);\n"
                "    synthlab.test.assertEquals(synthlab.regions.getGain(t, r), 0.123, \"gain after set\");\n"
                "    synthlab.actions.undo();\n"
                "    synthlab.test.assertEquals(synthlab.regions.getGain(t, r), before, \"gain after undo\");\n"
                "    console.log(\"PASS: gain roundtrip\");\n"
                "}\n");
    });
    templates.addItem ("Instrument-Track + Clip + Record (full workflow)", [insert]() {
        insert ("// Full workflow: pick plugin -> create instrument track -> open\n"
                "// Tracks tab -> create empty MIDI clip 1 bar long -> shift it one\n"
                "// bar forward -> arm the track -> turn metronome on -> hit record.\n"
                "var plugs = synthlab.plugins.list();\n"
                "if (plugs.length === 0) { alert(\"No instrument plugins scanned.\"); }\n"
                "else {\n"
                "    var name = prompt(\"Plugin?\", plugs[0]);\n"
                "    if (name !== null) {\n"
                "        var t = synthlab.actions.addInstrumentTrack(name);\n"
                "        if (t < 0) { alert(\"Failed: \" + name); }\n"
                "        else {\n"
                "            synthlab.ui.openTrackView();\n"
                "            // 1 bar in 4/4 at current BPM, in seconds.\n"
                "            var barSec = 4 * 60 / synthlab.transport.getBpm();\n"
                "            var r = synthlab.actions.addMidiClip(t, 0, barSec);\n"
                "            if (r >= 0) synthlab.actions.moveRegion(t, r, barSec);\n"
                "            synthlab.tracks.setRecording(t, true);\n"
                "            synthlab.transport.setMetronome(true);\n"
                "            synthlab.transport.setRecording(true);\n"
                "            synthlab.transport.play();\n"
                "            console.log(\"Recording \" + name + \" on track \" + t);\n"
                "        }\n"
                "    }\n"
                "}\n");
    });
    menu.addSubMenu ("Templates", templates);

    menu.showMenuAsync (PopupMenu::Options().withTargetComponent (&snippetsButton));
}

void JavaScriptEditorComponent::newFile()
{
    document.replaceAllContent ("");
    currentFile = File();
    updateTitle();
}

void JavaScriptEditorComponent::openFile()
{
    // Macros-Ordner als Standard-Anker (siehe saveFileAs).
    auto macroDir = File::getSpecialLocation (File::userHomeDirectory)
                        .getChildFile (".Synthlab/macros");
    if (! macroDir.exists())
        macroDir.createDirectory();

    auto initial = currentFile.existsAsFile()
                       ? currentFile.getParentDirectory()
                       : macroDir;

    chooser = std::make_unique<FileChooser> ("Open macro", initial, "*.js");

    auto flags = FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles;

    chooser->launchAsync (flags, [this] (const FileChooser& fc)
    {
        auto f = fc.getResult();
        if (f.existsAsFile())
            loadFromFile (f);
    });
}

void JavaScriptEditorComponent::loadFromFile (const File& f)
{
    document.replaceAllContent (f.loadFileAsString());
    currentFile = f;
    updateTitle();
    appendOutput ("Loaded " + f.getFullPathName() + "\n", Colours::cornflowerblue);
}

void JavaScriptEditorComponent::saveFile()
{
    if (! currentFile.existsAsFile())
    {
        saveFileAs();
        return;
    }

    writeToFile (currentFile);
}

void JavaScriptEditorComponent::saveFileAs()
{
    // Default ins Macros-Verzeichnis - Skripte aus dem Editor sind aus Sicht
    // des Users sowieso immer Makros (die werden vom Macros-Menü gefunden).
    // Liegt die aktuell offene Datei schon woanders, bleibt deren Verzeichnis
    // als Anker, damit "Save As" als Quick-Variant zum Umbenennen funktioniert.
    auto macroDir = File::getSpecialLocation (File::userHomeDirectory)
                        .getChildFile (".Synthlab/macros");
    if (! macroDir.exists())
        macroDir.createDirectory();

    auto initial = currentFile.existsAsFile()
                       ? currentFile.getParentDirectory()
                       : macroDir;

    chooser = std::make_unique<FileChooser> ("Save as macro", initial, "*.js");

    auto flags = FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles
                 | FileBrowserComponent::warnAboutOverwriting;

    chooser->launchAsync (flags, [this] (const FileChooser& fc)
    {
        auto f = fc.getResult();
        if (f != File())
        {
            if (f.getFileExtension().isEmpty())
                f = f.withFileExtension ("js");

            currentFile = f;
            writeToFile (f);
            updateTitle();
        }
    });
}

void JavaScriptEditorComponent::writeToFile (const File& f)
{
    if (f.replaceWithText (document.getAllContent()))
        appendOutput ("Saved " + f.getFullPathName() + "\n", Colours::cornflowerblue);
    else
        appendOutput ("Failed to save " + f.getFullPathName() + "\n", Colours::tomato);
}

void JavaScriptEditorComponent::updateTitle()
{
    titleLabel.setText (currentFile.existsAsFile() ? currentFile.getFileName() : "Untitled.js",
                        dontSendNotification);
}

void JavaScriptEditorComponent::appendOutput (const String& text, Colour colour)
{
    auto append = [this, text, colour]()
    {
        output.setColour (TextEditor::textColourId, colour);
        output.moveCaretToEnd();
        output.insertTextAtCaret (text);
    };

    if (MessageManager::getInstance()->isThisTheMessageThread())
        append();
    else
        MessageManager::callAsync (append);
}

void JavaScriptEditorComponent::runScript()
{
    runScriptText (document.getAllContent(), "script");
}

bool JavaScriptEditorComponent::runFile (const File& file)
{
    if (! file.existsAsFile())
    {
        appendOutput ("Macro file not found: " + file.getFullPathName() + "\n",
                      Colours::tomato);
        return false;
    }
    return runScriptText (file.loadFileAsString(), file.getFileName());
}

bool JavaScriptEditorComponent::runScriptText (const String& script, const String& label)
{
    duk_context* ctx = duk_create_heap_default();
    if (ctx == nullptr)
    {
        appendOutput ("Could not create Duktape heap.\n", Colours::tomato);
        return false;
    }

    currentEditor = this;

    // global print()
    duk_push_c_function (ctx, native_print, DUK_VARARGS);
    duk_put_global_string (ctx, "print");

    // global console object with .log() and .error()
    duk_push_object (ctx);
    duk_push_c_function (ctx, native_console_log, DUK_VARARGS);
    duk_put_prop_string (ctx, -2, "log");
    duk_push_c_function (ctx, native_console_error, DUK_VARARGS);
    duk_put_prop_string (ctx, -2, "error");
    duk_put_global_string (ctx, "console");

    // Browser-style modal dialog functions - both block until dismissed.
    duk_push_c_function (ctx, native_alert,  DUK_VARARGS);
    duk_put_global_string (ctx, "alert");
    duk_push_c_function (ctx, native_prompt, DUK_VARARGS);
    duk_put_global_string (ctx, "prompt");

    registerSynthlabApi (ctx);

    appendOutput ("--- Running " + label + " ---\n", Colours::cornflowerblue);

    bool ok = true;
    if (duk_peval_string (ctx, script.toRawUTF8()) != 0)
    {
        // peval leaves the error on the top of the stack.
        const char* err = duk_safe_to_string (ctx, -1);
        appendOutput ("Error: " + String::fromUTF8 (err) + "\n", Colours::tomato);
        ok = false;
    }
    else
    {
        // Show the script's return value (if any) - useful for `2 + 2` style
        // expressions where there's no explicit print.
        if (! duk_is_undefined (ctx, -1))
        {
            const char* result = duk_safe_to_string (ctx, -1);
            appendOutput ("=> " + String::fromUTF8 (result) + "\n",
                          Colours::lightgreen);
        }
    }

    duk_pop (ctx);
    duk_destroy_heap (ctx);
    currentEditor = nullptr;

    appendOutput ("--- Done ---\n", Colours::cornflowerblue);
    return ok;
}
