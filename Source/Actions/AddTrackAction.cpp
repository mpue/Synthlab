/*
  ==============================================================================

    AddTrackAction.cpp
    Created: 16 May 2026
    Author:  mpue

  ==============================================================================
*/

#include "AddTrackAction.h"
#include "AddModuleAction.h"

#include "../SynthEditor.h"
#include "../Module.h"
#include "../Pin.h"
#include "../Connection.h"
#include "../Project/Project.h"
#include "../Settings.h"
#include "../Mixer.h"
#include "../MixerPanel.h"
#include "../TrackUtil.h"
#include "../AudioManager.h"

#include "../Tracks/Track.h"
#include "../Tracks/TrackNavigator.h"
#include "../TrackIn.h"
#include "../AudioIn.h"
#include "../AudioOut.h"
#include "../AuxOut.h"
#include "../MidiGate.h"
#include "../MidiOut.h"
#include "../PluginModule.h"

#include <algorithm>

using juce::Point;
using juce::String;

namespace {
    // Prefab IDs used by PrefabFactory — same as in TrackUtil.
    constexpr long kPrefabAudioIn      = 76;
    constexpr long kPrefabAudioOut     = 66;
    constexpr long kPrefabAuxOut       = 77;
    constexpr long kPrefabTrackIn      = 48;
    constexpr long kPrefabMidiGate     = 61;
    constexpr long kPrefabMidiOut      = 62;
    constexpr long kPrefabPluginModule = 79;
}

AddTrackAction::AddTrackAction(SynthEditor* editor_,
                               TrackNavigator* navigator_,
                               Type type_,
                               juce::String instrumentName_)
    : editor(editor_), navigator(navigator_), type(type_), instrumentName(instrumentName_)
{
}

AddTrackAction::~AddTrackAction()
{
    if (!attached) {
        // We own the Track + Modules + Connections while they're detached.
        // Connections are independent of the module graph here, delete first.
        for (auto* c : connections) delete c;
        for (auto* m : modules)     delete m;
        delete track;
    }
}

// ---------------------------------------------------------------------------

Module* AddTrackAction::addModuleNow(Point<int> pos, long prefabId)
{
    // Use a throwaway AddModuleAction to piggyback on its mixer-registration
    // and visibility plumbing. We do NOT push the sub-action to the UndoManager
    // — that would produce parasitic undo steps that race with our parent
    // transaction. We just want the side-effects of perform().
    AddModuleAction tmp(editor, pos, prefabId);
    if (!tmp.perform())
        return nullptr;
    Module* m = tmp.getModule();
    if (m != nullptr)
        modules.push_back(m);
    return m;
}

void AddTrackAction::wireConnection(Module* srcMod, int srcPinIdx,
                                    Module* dstMod, int dstPinIdx,
                                    bool /*backLinkOnDst*/)
{
    if (srcMod == nullptr || dstMod == nullptr) return;
    if (srcPinIdx < 0 || srcPinIdx >= (int) srcMod->pins.size()) return;
    if (dstPinIdx < 0 || dstPinIdx >= (int) dstMod->pins.size()) return;

    Pin* src = srcMod->pins.at(srcPinIdx);
    Pin* dst = dstMod->pins.at(dstPinIdx);

    // Back-link convention used throughout the codebase (see AddConnectionAction
    // and TrackUtil): the consumer pin holds a pointer to the producer pin in
    // its getConnections() list. Connection's source = producer.
    dst->getConnections().push_back(src);
    auto* c = new Connection(srcMod, src, dstMod, dst);
    editor->getModule()->addConnection(c);

    pinLinks.push_back({ dst, src });
    connections.push_back(c);
}

// ---------------------------------------------------------------------------

bool AddTrackAction::perform()
{
    // Hold the writer lock for the whole transaction. buildAudio/Midi/
    // Instrument call addModuleNow() which delegates to AddModuleAction::
    // perform() (which also takes this lock — recursive CriticalSection is
    // fine), and wireConnection() push_backs into pin / root-connection
    // vectors that the audio thread iterates under its ScopedTryLock. Same
    // reasoning applies to doAttach() on the redo path.
    const juce::ScopedLock graphLock(
        AudioManager::getInstance()->getProcessGraphLock());

    if (!built) {
        switch (type) {
            case Type::Audio:      buildAudio(); break;
            case Type::Midi:       buildMidi(); break;
            case Type::Instrument: buildInstrument(); break;
        }
        built    = (track != nullptr);
        attached = built;
        if (editor != nullptr) editor->resized();
        return built;
    }

    // Redo
    doAttach();
    if (editor != nullptr) editor->resized();
    return true;
}

bool AddTrackAction::undo()
{
    if (!built) return false;

    // Mirror perform(): doDetach() erases from module / connection / mixer
    // vectors that the audio thread is walking — hold the writer lock so
    // the callback drops a buffer rather than touching freed storage.
    const juce::ScopedLock graphLock(
        AudioManager::getInstance()->getProcessGraphLock());

    doDetach();
    if (editor != nullptr) editor->resized();
    return true;
}

// ---------------------------------------------------------------------------

void AddTrackAction::buildAudio()
{
    int audioTrackCount = 0;
    for (auto* t : navigator->getTracks())
        if (t != nullptr && t->getType() == Track::Type::AUDIO) ++audioTrackCount;
    const int leftChan  = audioTrackCount * 2;
    const int rightChan = audioTrackCount * 2 + 1;

    Point<int> origin = editor->getNextTrackStartPosition();
    Point<int> pos    = origin;

    auto* inL = dynamic_cast<AudioIn*>(addModuleNow(pos, kPrefabAudioIn));
    if (inL != nullptr) inL->setSelectedAudioInput(leftChan);

    pos.y += 155;
    auto* inR = dynamic_cast<AudioIn*>(addModuleNow(pos, kPrefabAudioIn));
    if (inR != nullptr) inR->setSelectedAudioInput(rightChan);

    pos = { origin.x + 170, origin.y + 70 };
    auto* ti = dynamic_cast<TrackIn*>(addModuleNow(pos, kPrefabTrackIn));

    track = new Track(Track::Type::AUDIO,
                       Project::getInstance()->getSampleRate(),
                       navigator->getDragger());
    track->setName("Audio " + String(navigator->getTracks().size() + 1));
    if (ti != nullptr) ti->setTrack(track);
    navigator->addTrack(track);   // also registers with the Mixer

    if (inL != nullptr) wireConnection(inL, 0, ti, 0, true);
    if (inR != nullptr) wireConnection(inR, 0, ti, 1, true);

    // Output module: AudioOut (default) re-uses the first existing AudioOut if
    // unwired, otherwise spawns an AuxOut so the track lands on its own strip.
    // AuxOut setting always spawns a fresh AuxOut.
    pos = { origin.x + 330, origin.y + 70 };

    const bool preferAuxOut =
        Settings::getInstance()->getNewTrackOutputType()
            == Settings::NewTrackOutputType::AuxOut;

    if (preferAuxOut) {
        auto* ao = dynamic_cast<AuxOut*>(addModuleNow(pos, kPrefabAuxOut));
        if (ao != nullptr && ti != nullptr) {
            wireConnection(ti, 2, ao, 0, true);
            wireConnection(ti, 3, ao, 1, true);
            destinationModule = ao;
        }
    } else {
        AudioOut* existing = nullptr;
        for (auto* m : *editor->getModule()->getModules()) {
            if ((existing = dynamic_cast<AudioOut*>(m)) != nullptr) break;
        }

        const bool existingTaken = (existing != nullptr)
            && (existing->pins.at(0)->getConnections().size() > 0
             || existing->pins.at(1)->getConnections().size() > 0);

        if (existing != nullptr && !existingTaken && ti != nullptr) {
            // Re-use existing AudioOut. We did NOT create it, so we don't add
            // it to `modules` — only the connection ties our track to it.
            wireConnection(ti, 2, existing, 0, true);
            wireConnection(ti, 3, existing, 1, true);
            existing->setVolume(1.0f);
            destinationModule = existing;
        } else if (existing == nullptr && ti != nullptr) {
            // No AudioOut at all yet — create one and wire it.
            auto* ao = dynamic_cast<AudioOut*>(addModuleNow(pos, kPrefabAudioOut));
            if (ao != nullptr) {
                wireConnection(ti, 2, ao, 0, true);
                wireConnection(ti, 3, ao, 1, true);
                ao->setVolume(1.0f);
                destinationModule = ao;
            }
        } else if (ti != nullptr) {
            // AudioOut is taken — fall back to a fresh AuxOut.
            auto* ao = dynamic_cast<AuxOut*>(addModuleNow(pos, kPrefabAuxOut));
            if (ao != nullptr) {
                wireConnection(ti, 2, ao, 0, true);
                wireConnection(ti, 3, ao, 1, true);
                destinationModule = ao;
            }
        }
    }

    if (track != nullptr && destinationModule != nullptr)
        TrackUtil::LinkTrackToMixerPanel(track, destinationModule, editor);
}

void AddTrackAction::buildMidi()
{
    const int channel = TrackUtil::getNextFreeMidiChannel(navigator);

    Point<int> pos = editor->getNextTrackStartPosition();

    auto* gate = dynamic_cast<MidiGate*>(addModuleNow(pos, kPrefabMidiGate));

    pos.x += 160;
    auto* ti = dynamic_cast<TrackIn*>(addModuleNow(pos, kPrefabTrackIn));

    track = new Track(Track::Type::MIDI,
                       Project::getInstance()->getSampleRate(),
                       navigator->getDragger());
    track->setName("MIDI " + String(navigator->getTracks().size() + 1));
    if (ti != nullptr) ti->setTrack(track);
    navigator->addTrack(track);

    if (gate != nullptr && ti != nullptr)
        wireConnection(ti, 4, gate, 1, true);

    pos.x += 160;
    auto* midiOut = dynamic_cast<MidiOut*>(addModuleNow(pos, kPrefabMidiOut));

    if (midiOut != nullptr) {
        track->setMidiChannel(channel, false);
        midiOut->setMidiChannel(channel);

        if (ti != nullptr)
            wireConnection(midiOut, 0, ti, 5, true);
    }
}

void AddTrackAction::buildInstrument()
{
    const int channel = TrackUtil::getNextFreeMidiChannel(navigator);

    Point<int> pos = editor->getNextTrackStartPosition();

    auto* gate = dynamic_cast<MidiGate*>(addModuleNow(pos, kPrefabMidiGate));
    if (gate != nullptr) gate->setChannel(channel);

    pos.x += 160;
    auto* ti = dynamic_cast<TrackIn*>(addModuleNow(pos, kPrefabTrackIn));

    track = new Track(Track::Type::INSTRUMENT,
                       Project::getInstance()->getSampleRate(),
                       navigator->getDragger());
    track->setType(Track::Type::INSTRUMENT);
    track->setInstrumentName(instrumentName);
    track->setName(instrumentName);
    if (ti != nullptr) ti->setTrack(track);
    navigator->addTrack(track);
    track->setVolume(1.0f);
    track->setMidiChannel(channel, false);

    if (gate != nullptr && ti != nullptr)
        wireConnection(ti, 4, gate, 1, true);

    pos.x += 160;
    auto* pm = dynamic_cast<PluginModule*>(addModuleNow(pos, kPrefabPluginModule));
    if (pm != nullptr) {
        pm->selectPlugin(instrumentName);
        pm->setChannel(channel);

        if (ti != nullptr)
            wireConnection(pm, 4, ti, 5, true);
    }

    pos.x += 160;
    if (pm != nullptr) track->setPluginModule(pm);

    const bool preferAuxOut =
        Settings::getInstance()->getNewTrackOutputType()
            == Settings::NewTrackOutputType::AuxOut;

    if (preferAuxOut) {
        auto* ao = dynamic_cast<AuxOut*>(addModuleNow(pos, kPrefabAuxOut));
        if (ao != nullptr && pm != nullptr) {
            wireConnection(pm, 2, ao, 0, true);
            wireConnection(pm, 3, ao, 1, true);
            destinationModule = ao;
        }
    } else {
        auto* ao = dynamic_cast<AudioOut*>(addModuleNow(pos, kPrefabAudioOut));
        if (ao != nullptr && pm != nullptr) {
            // Mirror TrackUtil::ConnectAudioOut: if the fresh AudioOut is
            // already wired (won't happen for a freshly-created one), fall back
            // to AuxOut. Otherwise wire directly.
            const bool taken = (ao->pins.at(0)->getConnections().size() > 0
                             || ao->pins.at(1)->getConnections().size() > 0);
            if (!taken) {
                wireConnection(pm, 2, ao, 0, true);
                wireConnection(pm, 3, ao, 1, true);
                ao->setVolume(1.0f);
                destinationModule = ao;
            } else {
                auto* aux = dynamic_cast<AuxOut*>(addModuleNow(pos, kPrefabAuxOut));
                if (aux != nullptr) {
                    wireConnection(pm, 2, aux, 0, true);
                    wireConnection(pm, 3, aux, 1, true);
                    destinationModule = aux;
                }
            }
        }
    }

    if (track != nullptr && destinationModule != nullptr)
        TrackUtil::LinkTrackToMixerPanel(track, destinationModule, editor);

    if (auto* pv = Project::getInstance()->getPropertyView())
        pv->pulseShowPluginButton(track);

    if (pm != nullptr && Settings::getInstance()->getOpenPluginWindowAutomatically())
        pm->openPluginWindow();
}

// ---------------------------------------------------------------------------

void AddTrackAction::doDetach()
{
    editor->setRunning(false);

    Module*                   root      = editor->getModule();
    std::vector<Module*>*     rootMods  = root->getModules();
    std::vector<Connection*>* rootConns = root->getConnections();

    // 1) Strip the pin back-links we added.
    for (auto& link : pinLinks) {
        auto& host = link.outer->getConnections();
        host.erase(std::remove(host.begin(), host.end(), link.inner), host.end());
    }

    // 2) Pull our Connection objects out of the root graph.
    for (auto* c : connections) {
        auto it = std::find(rootConns->begin(), rootConns->end(), c);
        if (it != rootConns->end()) rootConns->erase(it);
    }

    // 3) Detach the modules we created (mirror RemoveSelectedAction's mixer-
    //    aware path so the Mixer doesn't keep stale channel strips around).
    auto* mixer = editor->getMixer().get();
    for (auto* m : modules) {
        if (mixer != nullptr) {
            if (auto* out = dynamic_cast<AudioOut*>(m)) {
                mixer->removeChannel(out->getChannelIndex());
                auto& outs = mixer->getOutputChannels();
                outs.erase(std::remove(outs.begin(), outs.end(), out), outs.end());
            } else if (auto* in = dynamic_cast<AudioIn*>(m)) {
                mixer->removeChannel(in->getChannelIndex());
                auto& ins = mixer->getInputChannels();
                ins.erase(std::remove(ins.begin(), ins.end(), in), ins.end());
            } else if (auto* aux = dynamic_cast<AuxOut*>(m)) {
                mixer->removeChannel(aux->getChannelIndex());
                auto& auxs = mixer->getAuxChannels();
                auxs.erase(std::remove(auxs.begin(), auxs.end(), aux), auxs.end());
            }
        }
        editor->removeChangeListener(m);
        editor->removeChildComponent(m);
        rootMods->erase(std::remove(rootMods->begin(), rootMods->end(), m), rootMods->end());
    }

    // 4) Detach the Track (navigator + mixer).
    if (track != nullptr) {
        int idx = -1;
        navigator->detachTrack(track, idx);
        navigatorIndex = idx;
        Mixer::getInstance()->removeTrack(track);
    }

    editor->setRunning(true);
    editor->repaint();
    attached = false;
}

void AddTrackAction::doAttach()
{
    editor->setRunning(false);

    Module*                   root      = editor->getModule();
    std::vector<Module*>*     rootMods  = root->getModules();
    std::vector<Connection*>* rootConns = root->getConnections();

    // 1) Re-attach the Track (navigator + mixer).
    if (track != nullptr) {
        navigator->insertTrackAt(track, navigatorIndex);
        Mixer::getInstance()->addTrack(track);
    }

    // 2) Re-attach modules. configureAudioModule handles the mixer re-
    //    registration for AudioIn/Out/Aux/Bus (same pattern as
    //    RemoveSelectedAction::undo).
    for (auto* m : modules) {
        rootMods->push_back(m);
        editor->addAndMakeVisible(m);
        editor->configureAudioModule(m, editor);
        m->setCurrentLayer(editor->getCurrentLayerEnum());
    }

    // 3) Restore pin back-links + Connection objects.
    for (auto& link : pinLinks)
        link.outer->getConnections().push_back(link.inner);

    for (auto* c : connections)
        rootConns->push_back(c);

    // 4) Re-link the mixer channel strip to the track.
    if (track != nullptr && destinationModule != nullptr)
        TrackUtil::LinkTrackToMixerPanel(track, destinationModule, editor);

    editor->setRunning(true);
    editor->repaint();
    attached = true;
}
