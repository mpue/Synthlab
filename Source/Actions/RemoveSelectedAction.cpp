/*
  ==============================================================================

    RemoveSelectedAction.cpp
    Created: 9 May 2018 2:41:55pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "RemoveSelectedAction.h"
#include "../AudioOut.h"
#include "../AudioIn.h"
#include "../AuxOut.h"
#include "../MixerPanel.h"
#include <algorithm>
#include <set>

RemoveSelectedAction::RemoveSelectedAction(SynthEditor* editor) {
    this->editor = editor;
}

RemoveSelectedAction::~RemoveSelectedAction() {
    if (!modulesInEditor) {
        for (auto* c : removedConnections) delete c;
        for (auto* m : removedModules)     delete m;
    }
}

bool RemoveSelectedAction::perform() {
    editor->setRunning(false);

    Module*                  rootMod  = editor->getModule();
    std::vector<Module*>*    rootMods = rootMod->getModules();
    std::vector<Connection*>* rootCons = rootMod->getConnections();
    MixerPanel*              mixer    = editor->getMixer().get();

    bool firstCall = removedModules.empty() && removedConnections.empty();

    if (firstCall) {
        for (auto* m : *rootMods)
            if (m->isSelected())
                removedModules.push_back(m);
    }

    std::set<long> removedIndices;
    for (auto* m : removedModules)
        removedIndices.insert(m->getIndex());

    if (firstCall) {
        // Collect all connections to be removed: explicitly selected, or touching a removed module
        for (auto it = rootCons->begin(); it != rootCons->end(); ) {
            Connection* c = *it;
            if (c->selected ||
                removedIndices.count(c->source->getIndex()) ||
                removedIndices.count(c->target->getIndex())) {
                removedConnections.push_back(c);
                it = rootCons->erase(it);
            } else {
                ++it;
            }
        }

        if (removedModules.empty() && removedConnections.empty()) {
            editor->setRunning(true);
            return false;
        }

        // Collect pin-level links from surviving modules pointing into removed modules
        for (auto* m : *rootMods) {
            if (removedIndices.count(m->getIndex())) continue;
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
        // Redo: re-remove connections and pin links that undo() restored
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

    // Remove modules from editor
    for (auto* m : removedModules) {
        if (mixer) {
            AudioOut* out = dynamic_cast<AudioOut*>(m);
            AudioIn*  in  = dynamic_cast<AudioIn*>(m);
            AuxOut*   aux = dynamic_cast<AuxOut*>(m);
            if (out) {
                mixer->removeChannel(out->getChannelIndex());
                auto& outs = mixer->getOutputChannels();
                outs.erase(std::remove(outs.begin(), outs.end(), out), outs.end());
            } else if (in) {
                mixer->removeChannel(in->getChannelIndex());
                auto& ins = mixer->getInputChannels();
                ins.erase(std::remove(ins.begin(), ins.end(), in), ins.end());
            } else if (aux) {
                mixer->removeChannel(aux->getChannelIndex());
                auto& auxs = mixer->getAuxChannels();
                auxs.erase(std::remove(auxs.begin(), auxs.end(), aux), auxs.end());
            }
        }
        editor->removeChangeListener(m);
        editor->removeChildComponent(m);
        rootMods->erase(std::remove(rootMods->begin(), rootMods->end(), m), rootMods->end());
    }

    modulesInEditor = false;
    editor->getSelectionModel().clearSelection();
    editor->setDirty(true);
    editor->setRunning(true);
    editor->repaint();
    return true;
}

bool RemoveSelectedAction::undo() {
    if (removedModules.empty() && removedConnections.empty()) return false;

    editor->setRunning(false);

    Module*                  rootMod  = editor->getModule();
    std::vector<Module*>*    rootMods = rootMod->getModules();
    std::vector<Connection*>* rootCons = rootMod->getConnections();

    for (auto* m : removedModules) {
        rootMods->push_back(m);
        editor->addAndMakeVisible(m);
        editor->configureAudioModule(m, editor);  // handles mixer re-registration + addChangeListener
        m->setCurrentLayer(editor->getCurrentLayerEnum());
    }

    for (auto& link : removedPinLinks)
        link.outer->getConnections().push_back(link.inner);

    for (auto* c : removedConnections)
        rootCons->push_back(c);

    modulesInEditor = true;
    editor->getSelectionModel().clearSelection();
    editor->setDirty(true);
    editor->setRunning(true);
    editor->repaint();
    return true;
}

void RemoveSelectedAction::removeSelectedItem() {
    perform();
}
