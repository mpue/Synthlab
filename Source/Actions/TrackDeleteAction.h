#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "../Tracks/TrackNavigator.h"
#include "../SynthEditor.h"
#include "../Pin.h"
#include "../Connection.h"

class Module;

// Removes a Track and every module that exists solely to wire it up — TrackIn
// plus its direct neighbours (AudioIn x 2 / MidiGate / AudioOut / AuxOut /
// MidiOut / PluginModule). A neighbour is removed only when ALL its
// connections lead back to the track's core modules; if the user wired
// something extra to the master AudioOut, the AudioOut stays put and we just
// drop our connection to it. The inverse of AddTrackAction.
class TrackDeleteAction : public juce::UndoableAction
{
public:
    // navigator + editor are required. `explicitTrack` lets callers (e.g. the
    // TrackIn-delete cascade) target a specific Track without first selecting
    // it; when null we fall back to detachSelectedTrack like before.
    TrackDeleteAction(TrackNavigator* navigator,
                      SynthEditor* editor,
                      Track* explicitTrack = nullptr);
    ~TrackDeleteAction() override;

    bool perform() override;
    bool undo() override;

private:
    struct PinLink { Pin* outer; Pin* inner; };

    TrackNavigator* navigator;
    SynthEditor*    editor;

    // Either explicitly supplied or grabbed via detachSelectedTrack on the
    // first perform.
    Track* explicitTrack = nullptr;
    Track* removedTrack  = nullptr;
    int    savedIndex    = -1;

    // The full module set we tore out — TrackIn, PluginModule (instrument),
    // and any exclusive satellites (AudioIn / MidiGate / MidiOut, plus
    // AudioOut/AuxOut when they're not shared with user-added wiring).
    std::vector<Module*>     removedModules;
    std::vector<Connection*> removedConnections;
    std::vector<PinLink>     removedPinLinks;

    // The output module that the mixer channel strip was linked to (AudioOut /
    // AuxOut). When set, undo re-establishes the panel<->track link.
    Module* destinationModule = nullptr;

    bool trackInNavigator = true;
    bool firstPerform     = true;

    void discoverAssembly();
    bool isTrackSatellite(Module* m) const;
    void detachModules();
    void attachModules();
};
