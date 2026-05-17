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

extern "C" {
    #include "duktape/duktape.h"
}

using namespace juce;

namespace
{
    // The print/console.log bridge runs synchronously on the same thread as
    // runScript(), so a thread-local pointer is enough to route output back
    // into the calling editor instance — no locking required.
    thread_local JavaScriptEditorComponent* currentEditor = nullptr;

    String collectAllArgsAsString (duk_context* ctx)
    {
        const int n = duk_get_top (ctx);
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

    // --- synthlab.transport bridge --------------------------------------
    //
    // runScript() is invoked from a Button::Listener callback, so we are on
    // the message thread when these natives fire. TrackNavigator's play /
    // stop / setPosition entry points are already called from key handlers
    // on the same thread, so calling them directly here is safe — no need
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

    // --- synthlab.tracks / synthlab.regions -----------------------------
    //
    // These are read-only views over the current TrackNavigator state.
    // Out-of-range indices return undefined / sensible defaults instead of
    // throwing — duk_error uses longjmp which would skip C++ destructors
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

    // Region helpers — start/length exposed in seconds using the project
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

    duk_ret_t native_actions_addTrackInternal (duk_context* ctx, AddTrackAction::Type type)
    {
        if (currentEditor == nullptr) { duk_push_int (ctx, -1); return 1; }

        auto* nav = currentEditor->getTransportNavigator();
        auto* ed  = currentEditor->getSynthEditor();
        if (nav == nullptr || ed == nullptr) { duk_push_int (ctx, -1); return 1; }

        auto* action = new AddTrackAction (ed, nav, type);
        auto* um     = Project::getInstance()->getUndoManager();
        um->beginNewTransaction();
        if (! um->perform (action)) { duk_push_int (ctx, -1); return 1; }

        // perform() owns the new track — find its index for the script.
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
            duk_put_prop_string (ctx, -2, "regions");

            // --- actions ------------------------------------------------
            duk_push_object (ctx);
            duk_push_c_function (ctx, native_actions_moveRegion,    3);
            duk_put_prop_string (ctx, -2, "moveRegion");
            duk_push_c_function (ctx, native_actions_addAudioTrack, 0);
            duk_put_prop_string (ctx, -2, "addAudioTrack");
            duk_push_c_function (ctx, native_actions_addMidiTrack,  0);
            duk_put_prop_string (ctx, -2, "addMidiTrack");
            duk_push_c_function (ctx, native_actions_deleteTrack,   1);
            duk_put_prop_string (ctx, -2, "deleteTrack");
            duk_put_prop_string (ctx, -2, "actions");
        }
        duk_put_global_string (ctx, "synthlab");
    }
}

// ============================================================================
// Autocomplete: schema + popup + CodeEditor subclass
// ============================================================================
//
// The schema mirrors registerSynthlabApi above by hand — single source of truth
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
                { "synthlab", "synthlab", "Synthlab API root",  -1 },
                { "console",  "console",  "Logging",            -1 },
                { "print",    "print()",  "(...args) → output",  1 },
            }},
            { "synthlab", {
                { "transport", "transport", "Playhead, BPM",      -1 },
                { "tracks",    "tracks",    "Read track state",   -1 },
                { "regions",   "regions",   "Read region state",  -1 },
                { "actions",   "actions",   "Undoable edits",     -1 },
            }},
            { "synthlab.transport", {
                { "play",        "play()",        "Start playback",          0 },
                { "stop",        "stop()",        "Stop playback",           0 },
                { "toggle",      "toggle()",      "Toggle play/stop",        0 },
                { "isPlaying",   "isPlaying()",   "→ bool",                  0 },
                { "getPosition", "getPosition()", "→ seconds",               0 },
                { "setPosition", "setPosition()", "(seconds)",               1 },
                { "getBpm",      "getBpm()",      "→ tempo",                 0 },
                { "setBpm",      "setBpm()",      "(value)",                 1 },
            }},
            { "synthlab.tracks", {
                { "count",       "count()",       "→ int",                            0 },
                { "list",        "list()",        "→ string[] names",                 0 },
                { "getName",     "getName()",     "(i) → string",                     1 },
                { "getType",     "getType()",     "(i) → audio|midi|instrument",      1 },
                { "isMute",      "isMute()",      "(i) → bool",                       1 },
                { "isSolo",      "isSolo()",      "(i) → bool",                       1 },
                { "getGain",     "getGain()",     "(i) → number",                     1 },
                { "getVolume",   "getVolume()",   "(i) → number",                     1 },
                { "getPan",      "getPan()",      "(i) → number",                     1 },
                { "isSelected",  "isSelected()",  "(i) → bool",                       1 },
                { "regionCount", "regionCount()", "(i) → int",                        1 },
            }},
            { "synthlab.regions", {
                { "getName",    "getName()",    "(t, r) → string",          1 },
                { "getStart",   "getStart()",   "(t, r) → seconds",         1 },
                { "getLength",  "getLength()", "(t, r) → seconds",          1 },
                { "isSelected", "isSelected()", "(t, r) → bool",            1 },
                { "isLoop",     "isLoop()",     "(t, r) → bool",            1 },
            }},
            { "synthlab.actions", {
                { "moveRegion",    "moveRegion()",    "(t, r, deltaSeconds) → bool",  1 },
                { "addAudioTrack", "addAudioTrack()", "→ new track index",            0 },
                { "addMidiTrack",  "addMidiTrack()",  "→ new track index",            0 },
                { "deleteTrack",   "deleteTrack()",   "(t) → bool",                   1 },
            }},
            { "console", {
                { "log",   "log()",   "(...args) → output",       1 },
                { "error", "error()", "(...args) → error output", 1 },
            }},
        };
        static const std::vector<ApiMember> empty;
        auto it = table.find (path);
        return it != table.end() ? it->second : empty;
    }
}  // anonymous namespace — classes follow at file scope so they match the
   // forward declarations in JavaScriptEditorComponent.h (forward-declaring
   // an anonymous-namespace type at global scope mismatches at link time).

    // ------------------------------------------------------------------------
    // Popup component: a thin ListBox wrapped in a panel, positioned at the
    // caret. Does NOT take keyboard focus — all navigation keys come through
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

        // Callback for the editor — runs on the message thread.
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

            // Ctrl-Space / Cmd-Space: open popup at caret for the current
            // context, even without having just typed a dot.
            if (k.getKeyCode() == juce::KeyPress::spaceKey
                && (k.getModifiers().isCommandDown() || k.getModifiers().isCtrlDown()))
            {
                triggerPopup();
                return true;
            }

            return juce::CodeEditorComponent::keyPressed (k);
        }

        // -- CodeDocument::Listener -----------------------------------------
        //
        // Trigger policy: popup opens only on '.' or Cmd/Ctrl-Space, never on
        // a bare letter — that policy keeps the popup unobtrusive during
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
    addAndMakeVisible (clearButton);

    newButton   .addListener (this);
    openButton  .addListener (this);
    saveButton  .addListener (this);
    saveAsButton.addListener (this);
    runButton   .addListener (this);
    clearButton .addListener (this);

    editor = std::make_unique<JavaScriptCodeEditor> (document, &tokeniser);
    editor->setColourScheme (tokeniser.getDefaultColourScheme());
    editor->setColour (CodeEditorComponent::backgroundColourId, Colour (0xff1e1e1e));
    editor->setColour (CodeEditorComponent::defaultTextColourId, Colours::white);
    editor->setColour (CodeEditorComponent::lineNumberBackgroundId, Colour (0xff2a2a2a));
    editor->setColour (CodeEditorComponent::lineNumberTextId, Colour (0xff7a7a7a));
    editor->setColour (CodeEditorComponent::highlightColourId, Colour (0xff3a3a55));
    editor->setFont (Font (Font::getDefaultMonospacedFontName(), 14.0f, Font::plain));
    addAndMakeVisible (editor.get());

    // Popup floats above the editor — added AFTER the editor so it paints on
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
        "//   synthlab.regions.isSelected(t, r) / isLoop(t, r)\n"
        "//\n"
        "//   synthlab.actions.moveRegion(t, r, deltaSeconds)\n"
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
    newButton   .removeListener (this);
    openButton  .removeListener (this);
    saveButton  .removeListener (this);
    saveAsButton.removeListener (this);
    runButton   .removeListener (this);
    clearButton .removeListener (this);
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

    // Popup positions itself at the caret on show — close it if the layout
    // shifts under it so the user doesn't see a stale floating menu.
    if (autoComplete != nullptr && autoComplete->isOpen())
        autoComplete->hide();
}

void JavaScriptEditorComponent::buttonClicked (Button* b)
{
    if (b == &newButton)        newFile();
    else if (b == &openButton)  openFile();
    else if (b == &saveButton)  saveFile();
    else if (b == &saveAsButton) saveFileAs();
    else if (b == &runButton)   runScript();
    else if (b == &clearButton) output.clear();
}

void JavaScriptEditorComponent::newFile()
{
    document.replaceAllContent ("");
    currentFile = File();
    updateTitle();
}

void JavaScriptEditorComponent::openFile()
{
    chooser = std::make_unique<FileChooser> ("Open JavaScript file",
                                             File::getSpecialLocation (File::userHomeDirectory),
                                             "*.js");

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
    auto initial = currentFile.existsAsFile()
                       ? currentFile.getParentDirectory()
                       : File::getSpecialLocation (File::userHomeDirectory);

    chooser = std::make_unique<FileChooser> ("Save JavaScript file", initial, "*.js");

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

void JavaScriptEditorComponent::runFile (const File& file)
{
    if (! file.existsAsFile())
    {
        appendOutput ("Macro file not found: " + file.getFullPathName() + "\n",
                      Colours::tomato);
        return;
    }
    runScriptText (file.loadFileAsString(), file.getFileName());
}

void JavaScriptEditorComponent::runScriptText (const String& script, const String& label)
{
    duk_context* ctx = duk_create_heap_default();
    if (ctx == nullptr)
    {
        appendOutput ("Could not create Duktape heap.\n", Colours::tomato);
        return;
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

    registerSynthlabApi (ctx);

    appendOutput ("--- Running " + label + " ---\n", Colours::cornflowerblue);

    if (duk_peval_string (ctx, script.toRawUTF8()) != 0)
    {
        // peval leaves the error on the top of the stack.
        const char* err = duk_safe_to_string (ctx, -1);
        appendOutput ("Error: " + String::fromUTF8 (err) + "\n", Colours::tomato);
    }
    else
    {
        // Show the script's return value (if any) — useful for `2 + 2` style
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
}
