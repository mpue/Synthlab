/*
 ==============================================================================
 
 MoveSelectedAction.cpp
 Created: 9 May 2018 2:41:55pm
 Author:  Matthias Pueski
 
 ==============================================================================
 */

#include "MoveSelectedAction.h"

MoveSelectedAction::MoveSelectedAction(SynthEditor* editor) {
    this->editor = editor;
}


bool MoveSelectedAction::perform() {
    
    for (int i = 0; i < editor->getSelectionModel().getSelectedModules().size(); i++) {
        
        Module* m = editor->getSelectionModel().getSelectedModules().at(i);
        
        if (editor->getCurrentLayer() == Module::Layer::GUI) {
            if (deltaX != 0 || deltaY != 0) {
                m->setUiPosition(m->getUiPosition().getX() + deltaX, m->getUiPosition().getY() + deltaY);
                m->saveUiPosition();
                deltaX = 0;
                deltaY = 0;
            }
            
            oldPositions.push_back(Point<int>(m->getSavedUiPosition()));
            m->saveUiPosition();
            movedModules.push_back(m);
        }
        else {
            if (deltaX != 0 || deltaY != 0) {
                m->setTopLeftPosition(m->getPosition().getX() + deltaX, m->getPosition().getY() + deltaY);
                m->savePosition();
                deltaX = 0;
                deltaY = 0;
            }
            oldPositions.push_back(Point<int>(m->getSavedPosition()));
            m->savePosition();
            movedModules.push_back(m);
        }
        

    }
    
    editor->setDirty(true);

    return true;
}

bool MoveSelectedAction::undo() {
    
    for (int i = 0; i < movedModules.size();i++) {
        if (editor->getCurrentLayer() == Module::Layer::GUI) {
            
            deltaX = abs(oldPositions.at(i).getX() - movedModules.at(i)->getUiPosition().getX());
            deltaY = abs(oldPositions.at(i).getY() - movedModules.at(i)->getUiPosition().getY());
            
            movedModules.at(i)->setUiPosition(oldPositions.at(i).getX(), oldPositions.at(i).getY());
            movedModules.at(i)->setTopLeftPosition(oldPositions.at(i).getX(), oldPositions.at(i).getY());
            movedModules.at(i)->saveUiPosition();
        }
        else {
            deltaX = abs(oldPositions.at(i).getX() - movedModules.at(i)->getPosition().getX());
            deltaY = abs(oldPositions.at(i).getY() - movedModules.at(i)->getPosition().getY());
            
            movedModules.at(i)->setTopLeftPosition(oldPositions.at(i).getX(), oldPositions.at(i).getY());
            movedModules.at(i)->savePosition();
        }
    }
    
    movedModules.clear();
    oldPositions.clear();
    editor->repaint();
    editor->resized();
    
    editor->setDirty(true);
    
    return true;
}




