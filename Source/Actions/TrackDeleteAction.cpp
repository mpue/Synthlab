#include "TrackDeleteAction.h"
#include "../PluginModule.h"
#include "../TrackIn.h"
#include "../AudioIn.h"
#include "../AudioOut.h"
#include "../AuxOut.h"
#include "../MidiGate.h"
#include "../MidiOut.h"
#include "../Mixer.h"
#include "../MixerPanel.h"
#include "../TrackUtil.h"
#include "../Module.h"
#include <algorithm>
#include <set>

namespace {
    bool isOutputModule(Module* m) {
        return dynamic_cast<AudioOut*>(m) != nullptr
            || dynamic_cast<AuxOut*>(m)   != nullptr
            || dynamic_cast<MidiOut*>(m)  != nullptr;
    }
}

TrackDeleteAction::TrackDeleteAction(TrackNavigator* navigator_,
                                     SynthEditor* editor_,
                                     Track* explicitTrack_)
    : navigator(navigator_), editor(editor_), explicitTrack(explicitTrack_)
{
}

TrackDeleteAction::~TrackDeleteAction()
{
    if (!trackInNavigator) {
        for (auto* c : removedConnections) delete c;
        for (auto* m : removedModules)     delete m;
        delete removedTrack;
    }
}

bool TrackDeleteAction::isTrackSatellite(Module* m) const
{
    return dynamic_cast<AudioIn*>(m)  != nullptr
        || dynamic_cast<MidiGate*>(m) != nullptr
        || dynamic_cast<AudioOut*>(m) != nullptr
        || dynamic_cast<AuxOut*>(m)   != nullptr
        || dynamic_cast<MidiOut*>(m)  != nullptr;
}

void TrackDeleteAction::discoverAssembly()
{
    Module* root = editor->getModule();
    auto* rootMods  = root->getModules();
    auto* rootConns = root->getConnections();

    // Core: TrackIn (always), plus PluginModule for instrument tracks.
    std::set<Module*> core;

    TrackIn* trackIn = nullptr;
    for (auto* m : *rootMods) {
        if (auto* ti = dynamic_cast<TrackIn*>(m)) {
            if (ti->getTrack() == removedTrack) {
                trackIn = ti;
                break;
            }
        }
    }
    if (trackIn != nullptr) {
        removedModules.push_back(trackIn);
        core.insert(trackIn);
    }

    if (removedTrack->getType() == Track::Type::INSTRUMENT) {
        if (auto* pm = removedTrack->getPluginModule()) {
            removedModules.push_back(pm);
            core.insert(pm);
        }
    }

    // Candidate satellites: modules connected to a core module.
    std::set<Module*> candidates;
    for (auto* c : *rootConns) {
        if (core.count(c->source) && !core.count(c->target))
            candidates.insert(c->target);
        else if (core.count(c->target) && !core.count(c->source))
            candidates.insert(c->source);
    }

    // Rule b: only adopt a satellite into the remove set if ALL of its
    // connections go back into our core. If the user wired something else
    // to a shared AudioOut, the AudioOut survives and we just unwire our
    // side via the connection cleanup pass.
    for (auto* cand : candidates) {
        if (!isTrackSatellite(cand)) continue;

        bool exclusive = true;
        for (auto* c : *rootConns) {
            Module* other = nullptr;
            if      (c->source == cand) other = c->target;
            else if (c->target == cand) other = c->source;
            else continue;

            if (other != nullptr && !core.count(other)) {
                exclusive = false;
                break;
            }
        }

        if (exclusive) {
            removedModules.push_back(cand);
            if (isOutputModule(cand))
                destinationModule = cand;
        }
    }
}

bool TrackDeleteAction::perform()
{
    // 1) Get hold of the Track (first call: detach from navigator).
    if (removedTrack == nullptr) {
        int idx = -1;
        if (explicitTrack != nullptr)
            removedTrack = navigator->detachTrack(explicitTrack, idx);
        else
            removedTrack = navigator->detachSelectedTrack(idx);

        if (removedTrack == nullptr) return false;
        savedIndex = idx;
        Mixer::getInstance()->removeTrack(removedTrack);
    } else {
        // Redo: track was re-inserted by undo. Detach by pointer this time.
        int idx = -1;
        navigator->detachTrack(removedTrack, idx);
        Mixer::getInstance()->removeTrack(removedTrack);
    }

    // 2) On first perform, work out the assembly (modules and connections).
    if (firstPerform) {
        discoverAssembly();
        firstPerform = false;
    }

    // 3) Detach every module + its connections + pin back-links.
    detachModules();

    trackInNavigator = false;
    return true;
}

bool TrackDeleteAction::undo()
{
    if (removedTrack == nullptr) return false;

    navigator->insertTrackAt(removedTrack, savedIndex);
    Mixer::getInstance()->addTrack(removedTrack);

    attachModules();

    // Re-link the channel strip to the track so FX/automation work again.
    if (destinationModule != nullptr)
        TrackUtil::LinkTrackToMixerPanel(removedTrack, destinationModule, editor);

    trackInNavigator = true;
    return true;
}

void TrackDeleteAction::detachModules()
{
    editor->setRunning(false);

    Module*                   root      = editor->getModule();
    std::vector<Module*>*     rootMods  = root->getModules();
    std::vector<Connection*>* rootConns = root->getConnections();

    std::set<Module*> removedSet(removedModules.begin(), removedModules.end());

    if (removedConnections.empty() && removedPinLinks.empty()) {
        // First call: collect any connection touching a removed module.
        for (auto it = rootConns->begin(); it != rootConns->end(); ) {
            Connection* c = *it;
            if (removedSet.count(c->source) || removedSet.count(c->target)) {
                removedConnections.push_back(c);
                it = rootConns->erase(it);
            } else {
                ++it;
            }
        }

        // Pin-level back-links from surviving modules into removed modules.
        for (auto* m : *rootMods) {
            if (removedSet.count(m)) continue;
            for (auto* pin : m->getPins()) {
                for (auto it2 = pin->getConnections().begin(); it2 != pin->getConnections().end(); ) {
                    bool pointsToRemoved = false;
                    for (auto* rm : removedModules) {
                        for (auto* rpin : rm->getPins()) {
                            if ((*it2)->index == rpin->index) {
                                pointsToRemoved = true;
                                break;
                            }
                        }
                        if (pointsToRemoved) break;
                    }
                    if (pointsToRemoved) {
                        removedPinLinks.push_back({pin, *it2});
                        it2 = pin->getConnections().erase(it2);
                    } else {
                        ++it2;
                    }
                }
            }
        }
    } else {
        // Redo: re-tear-down what undo() restored.
        for (auto& link : removedPinLinks) {
            auto& cs = link.outer->getConnections();
            cs.erase(std::remove(cs.begin(), cs.end(), link.inner), cs.end());
        }
        for (auto* c : removedConnections) {
            auto it = std::find(rootConns->begin(), rootConns->end(), c);
            if (it != rootConns->end()) rootConns->erase(it);
        }
    }

    // Remove modules from editor + mixer (RemoveSelectedAction pattern).
    auto* mixer = editor->getMixer().get();
    for (auto* m : removedModules) {
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

    editor->setRunning(true);
    editor->repaint();
}

void TrackDeleteAction::attachModules()
{
    editor->setRunning(false);

    Module*                   root      = editor->getModule();
    std::vector<Module*>*     rootMods  = root->getModules();
    std::vector<Connection*>* rootConns = root->getConnections();

    for (auto* m : removedModules) {
        rootMods->push_back(m);
        editor->addAndMakeVisible(m);
        editor->configureAudioModule(m, editor);
        m->setCurrentLayer(editor->getCurrentLayerEnum());
    }

    for (auto& link : removedPinLinks)
        link.outer->getConnections().push_back(link.inner);

    for (auto* c : removedConnections)
        rootConns->push_back(c);

    editor->setRunning(true);
    editor->repaint();
}
