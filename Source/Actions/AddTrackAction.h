/*
  ==============================================================================

    AddTrackAction.h
    Created: 16 May 2026
    Author:  mpue

    Atomic, undoable creation of an Audio / Midi / Instrument track. Builds
    the same module assembly as TrackUtil::createXTrack — Track plus TrackIn,
    AudioIn/MidiGate inputs, AudioOut/AuxOut/MidiOut/PluginModule outputs,
    plus all the wiring — and exposes it as one UndoableAction so a single
    Cmd-Z removes the lot.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <vector>

class SynthEditor;
class TrackNavigator;
class Track;
class Module;
class Pin;
class Connection;

class AddTrackAction : public juce::UndoableAction
{
public:
    enum class Type { Audio, Midi, Instrument };

    AddTrackAction(SynthEditor* editor,
                   TrackNavigator* navigator,
                   Type type,
                   juce::String instrumentName = {});
    ~AddTrackAction() override;

    bool perform() override;
    bool undo() override;

    Track* getTrack() const { return track; }

private:
    // The wiring back-ref convention used throughout the graph: the consumer
    // pin (`outer`) holds a pointer to the producer pin (`inner`) in its
    // getConnections() list. Tracked here so undo can erase both the back-ref
    // and the Connection* in lockstep.
    struct PinLink { Pin* outer; Pin* inner; };

    SynthEditor*    editor    = nullptr;
    TrackNavigator* navigator = nullptr;
    Type            type;
    juce::String    instrumentName;

    Track*  track             = nullptr;
    int     navigatorIndex    = -1;
    Module* destinationModule = nullptr; // AudioOut/AuxOut/MidiOut linked to the mixer strip

    // Modules we created — re-attached/detached as a unit. Excludes modules we
    // only wired to (e.g. a pre-existing AudioOut on the master strip).
    std::vector<Module*>     modules;
    std::vector<Connection*> connections;
    std::vector<PinLink>     pinLinks;

    bool built    = false;
    bool attached = false;

    void buildAudio();
    void buildMidi();
    void buildInstrument();

    Module* addModuleNow(juce::Point<int> pos, long prefabId);
    void    wireConnection(Module* srcMod, int srcPinIdx,
                           Module* dstMod, int dstPinIdx,
                           bool backLinkOnDst);

    void doDetach();
    void doAttach();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AddTrackAction)
};
