//
//  AddConnectionAction.cpp
//  Synthlab - App
//
//  Created by Matthias Pueski on 18.04.18.
//

#include "AddConnectionAction.h"

AddConnectionAction::AddConnectionAction(SynthEditor* editor, Module* module) {
    this->editor = editor;
    this->source = module;
    this->root = editor->getModule();
    
}

bool AddConnectionAction::perform() {

    Logger::writeToLog("Adding connection.");
    
    // find the selected input of the source module
    for (int j = 0; j < source->pins.size(); j++) {
        
        if (source->pins.at(j)->isSelected()) {
            a = source->pins.at(j);
            break;
        }
        
    }
    
    // now find the target module
    for (int i = 0; i < root->getModules()->size(); i++) {

        Module* m = root->getModules()->at(i);

        // Self-skip by POINTER, not by index. Index-based skip used to mask
        // any sibling that happened to share `source`'s index — in practice
        // exactly the "two non-prefab groups both at index 0" case from
        // legacy saves. Drag from one to the other silently produced
        // b == nullptr and the wire never appeared.
        if (m == source) {
            continue;
        }
        
        // find the selected input of the target  module
        for (int j = 0; j < m->pins.size(); j++) {
            
            if (m->pins.at(j)->isSelected()) {
                b = m->pins.at(j);
                target = m;
                break;
            }
            
        }
        
    }
    
    if (source != nullptr && target != nullptr && a != nullptr && b != nullptr) {

        if (a->getType() == b->getType()) {

            // Helper: only push the Pin*-back-reference if it isn't already there.
            // Reason: re-dragging an already-existing connection used to push the
            // same pin twice into the consumer's connections vector, which then
            // shows as "input doubly assigned" and leaks the runtime connection
            // when the visible Connection is later removed (the second back-ref
            // survives because removeConnection only erases one match).
            auto linkOnce = [](Pin* consumer, Pin* producer) {
                if (!consumer->hasConnection(producer))
                    consumer->getConnections().push_back(producer);
            };

            // Helper: don't add a second Connection* if one already wires the
            // same a/b/source/target. Mirrors ModuleUtils::connectionExists used
            // on the load path.
            auto rootHasConnection = [this](Pin* aa, Pin* bb) -> bool {
                for (auto* d : *root->getConnections()) {
                    if (d == nullptr) continue;
                    if ((d->a == aa && d->b == bb) || (d->a == bb && d->b == aa))
                        return true;
                }
                return false;
            };

            const bool duplicate = rootHasConnection(a, b);

            // for event based pins the direction is from source to target
            if (a->getType() == Pin::Type::EVENT ) {
                if (a->direction == Pin::Direction::IN && b->direction == Pin::Direction::OUT) {
                    linkOnce(b, a);
                    if (!duplicate) {
                        c = new Connection(source, a, target, b);
                        root->getConnections()->push_back(c);
                    }
                }
                else if (a->direction == Pin::Direction::OUT && b->direction == Pin::Direction::IN) {
                    linkOnce(a, b);
                    if (!duplicate) {
                        c = new Connection(source, a, target, b);
                        root->getConnections()->push_back(c);
                    }
                }
            }
            // in all other cases its the opposite direction
            else {
                if (a->direction == Pin::Direction::IN && b->direction == Pin::Direction::OUT) {
                    linkOnce(a, b);
                    if (!duplicate) {
                        c = new Connection(source, a, target, b);
                        root->getConnections()->push_back(c);
                    }
                }
                else if (a->direction == Pin::Direction::OUT && b->direction == Pin::Direction::IN) {
                    linkOnce(b, a);
                    if (!duplicate) {
                        c = new Connection(source, a, target, b);
                        root->getConnections()->push_back(c);
                    }
                }
            }

        }

    }
    
    editor->setDirty(true);
    
    return true;
}


bool AddConnectionAction::undo() {

    // perform() may have decided the requested connection was already present
    // (duplicate) and not created a new Connection. In that case undo must be a
    // no-op — otherwise we'd tear down the original wire that's still in use.
    if (c == nullptr) {
        return true;
    }

    for (std::vector<Pin*>::iterator ita = a->getConnections().begin();ita != a->getConnections().end();) {
        if ((*ita) == b) {
            ita = a->getConnections().erase(ita);
        }
        else {
            ++ita;
        }
    }
    
    for (std::vector<Pin*>::iterator itb = b->getConnections().begin();itb != b->getConnections().end();) {
        if ((*itb) == a) {
            itb = b->getConnections().erase(itb);
        }
        else {
            ++itb;
        }
    }
    
    for (std::vector<Connection*>::iterator it = root->getConnections()->begin();it != root->getConnections()->end();) {
        if ((*it) == c) {
            
            delete (*it);
            it = root->getConnections()->erase(it);
        }
        else {
            ++it;
        }
        
    }
    
    editor->repaint();
    
    return true;
}
