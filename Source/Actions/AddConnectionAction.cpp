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
        
        if (m->getIndex() == source->getIndex()) {
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
            
            // for event based pins the direction is from source to target
            if (a->getType() == Pin::Type::EVENT ) {
                if (a->direction == Pin::Direction::IN && b->direction == Pin::Direction::OUT) {
                    
                    b->getConnections().push_back(a);
                    c = new Connection(source, a, target, b);
                    root->getConnections()->push_back(c);
                    
                }
                else if (a->direction == Pin::Direction::OUT && b->direction == Pin::Direction::IN) {
                    a->getConnections().push_back(b);
                    c = new Connection(source, a, target, b);
                    root->getConnections()->push_back(c);
                }
            }
            // in all other cases its the opposite direction
            else {
                if (a->direction == Pin::Direction::IN && b->direction == Pin::Direction::OUT) {
                    a->getConnections().push_back(b);
                    c = new Connection(source, a, target, b);
                    root->getConnections()->push_back(c);
                }
                else if (a->direction == Pin::Direction::OUT && b->direction == Pin::Direction::IN) {
                    b->getConnections().push_back(a);
                    
                    c = new Connection(source, a, target, b);
                    root->getConnections()->push_back(c);
                }
            }
    
        }
        
    }
    
    
    
    return true;
}


bool AddConnectionAction::undo() {
    
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
