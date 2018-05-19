/*
  ==============================================================================

    RemoveSelectedAction.cpp
    Created: 9 May 2018 2:41:55pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "RemoveSelectedAction.h"

RemoveSelectedAction::RemoveSelectedAction(SynthEditor* editor) {
    this->editor = editor;
}


bool RemoveSelectedAction::perform() {
    removeSelectedItem();
    return true;
}

bool RemoveSelectedAction::undo() {
    return false;
}


void RemoveSelectedAction::removeSelectedItem() {
    editor->setRunning(false);
    std::vector<Connection*>* cons = editor->getModule()->getConnections();
    cons->erase(std::remove_if(cons->begin(), cons->end(), [](Connection* c){
        if (c->selected){
            delete c;
            return true;
        }
        return false;
    }),cons->end());
    
    for(std::vector<Module*>::iterator it = editor->getModule()->getModules()->begin();it != editor->getModule()->getModules()->end();it++) {
        if ((*it)->isSelected())
            ModuleUtils::removeModule(editor->getModule(), (*it),editor->getMixer(), editor);
    }
    for(std::vector<Module*>::iterator it = editor->getModule()->getModules()->begin();it != editor->getModule()->getModules()->end();) {
        if ((*it)->isSelected()) {
            delete *it;
            it = editor->getModule()->getModules()->erase(it);
        }
        else {
            ++it;
        }   
    }
    editor->getSelectionModel().clearSelection();
    editor->setDirty(true);
    editor->setRunning(true);
}


