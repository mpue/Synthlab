#include "TrackDeleteAction.h"
#include "../PluginModule.h"
#include "../Mixer.h"
#include <algorithm>
#include <set>

TrackDeleteAction::TrackDeleteAction(TrackNavigator* navigator, SynthEditor* editor)
    : navigator(navigator), editor(editor)
{
}

TrackDeleteAction::~TrackDeleteAction()
{
    if (!trackInNavigator) {
        for (auto* c : removedConnections) delete c;
        delete removedTrack;
        // removedModule is owned by the SynthEditor module graph when present;
        // when removed it is also owned by us (not in editor), so delete it too.
        // But note: removedModule is a child of SynthEditor (Component), so
        // deleteAllChildren would handle it. We delete it explicitly here since
        // we removed it from the component hierarchy.
        delete removedModule;
    }
}

bool TrackDeleteAction::perform()
{
    if (removedTrack == nullptr) {
        // First call: find the selected track
        int idx = -1;
        removedTrack = navigator->detachSelectedTrack(idx);
        if (removedTrack == nullptr)
            return false;
        savedIndex = idx;

        Mixer::getInstance()->removeTrack(removedTrack);

        // Handle the associated PluginModule for INSTRUMENT tracks
        if (removedTrack->getType() == Track::INSTRUMENT) {
            removedModule = removedTrack->getPluginModule();
        }
    } else {
        // Redo: track was previously detached; re-detach (it was re-inserted by undo)
        removedTrack->setSelected(true);
        int idx = -1;
        navigator->detachSelectedTrack(idx);
        Mixer::getInstance()->removeTrack(removedTrack);
    }

    if (removedModule != nullptr) {
        editor->setRunning(false);

        Module* rootMod  = editor->getModule();
        std::vector<Module*>*     rootMods = rootMod->getModules();
        std::vector<Connection*>* rootCons = rootMod->getConnections();

        std::set<long> removedIndex;
        removedIndex.insert(removedModule->getIndex());

        if (removedConnections.empty() && removedPinLinks.empty()) {
            // First call: collect connections touching this module
            for (auto it = rootCons->begin(); it != rootCons->end(); ) {
                Connection* c = *it;
                if (removedIndex.count(c->source->getIndex()) ||
                    removedIndex.count(c->target->getIndex())) {
                    removedConnections.push_back(c);
                    it = rootCons->erase(it);
                } else {
                    ++it;
                }
            }

            // Collect pin-level back-links from surviving modules into this module
            for (auto* m : *rootMods) {
                if (removedIndex.count(m->getIndex())) continue;
                for (auto* pin : m->getPins()) {
                    for (auto it2 = pin->getConnections().begin(); it2 != pin->getConnections().end(); ) {
                        bool pointsToRemoved = false;
                        for (auto* rpin : removedModule->getPins()) {
                            if ((*it2)->index == rpin->index) {
                                pointsToRemoved = true;
                                break;
                            }
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
            // Redo: re-remove connections that undo() restored
            for (auto& link : removedPinLinks) {
                auto& conns = link.outer->getConnections();
                auto  it2   = std::find(conns.begin(), conns.end(), link.inner);
                if (it2 != conns.end()) conns.erase(it2);
            }
            for (auto* c : removedConnections) {
                auto it = std::find(rootCons->begin(), rootCons->end(), c);
                if (it != rootCons->end()) rootCons->erase(it);
            }
        }

        editor->removeChangeListener(removedModule);
        editor->removeChildComponent(removedModule);
        rootMods->erase(std::remove(rootMods->begin(), rootMods->end(), removedModule), rootMods->end());

        editor->setRunning(true);
        editor->repaint();
    }

    trackInNavigator = false;
    return true;
}

bool TrackDeleteAction::undo()
{
    if (removedTrack == nullptr) return false;

    navigator->insertTrackAt(removedTrack, savedIndex);
    Mixer::getInstance()->addTrack(removedTrack);

    if (removedModule != nullptr) {
        editor->setRunning(false);

        Module* rootMod  = editor->getModule();
        std::vector<Module*>*     rootMods = rootMod->getModules();
        std::vector<Connection*>* rootCons = rootMod->getConnections();

        rootMods->push_back(removedModule);
        editor->addAndMakeVisible(removedModule);
        editor->configureAudioModule(removedModule, editor);
        removedModule->setCurrentLayer(editor->getCurrentLayerEnum());

        for (auto& link : removedPinLinks)
            link.outer->getConnections().push_back(link.inner);

        for (auto* c : removedConnections)
            rootCons->push_back(c);

        editor->setRunning(true);
        editor->repaint();
    }

    trackInNavigator = true;
    return true;
}
