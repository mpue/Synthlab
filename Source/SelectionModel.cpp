//
//  SelectionModel.cpp
//  Synthlab - App
//
//  Created by Matthias Pueski on 20.04.18.
//

#include "SelectionModel.h"
#include "Connection.h"

SelectionModel::SelectionModel() {
}

SelectionModel::~SelectionModel() {
    selectedModules.clear();
}

void SelectionModel::clearSelection() {
    
    for (int i = 0; i < root->getModules()->size();i++) {
        root->getModules()->at(i)->setSelected(false);
        root->getModules()->at(i)->setEditing(false);
    }
    
    getSelectedModules().clear();
}

void SelectionModel::checkForPinSelection(juce::Point<int> position) {
    
    for (int i = 0; i < root->getModules()->size();i++) {
    
         Module* m = root->getModules()->at(i);
        
        for (int j = 0; j < m->pins.size(); j++) {
            
            if (m->isMouseOverPin(j, position)) {
                m->pins.at(j)->setSelected(true);
                m->repaint();
                break;
            }
        }
    }

}

Module* SelectionModel::getSelectedModule() {
    if (root != NULL && root != nullptr) {
        for (int i = 0; i< root->getModules()->size();i++) {
            if (root->getModules()->at(i) != NULL && root->getModules()->at(i)->isSelected()) {
                return root->getModules()->at(i);
            }
        }
    }
    return nullptr;
}

std::vector<Module*>& SelectionModel::getSelectedModules() {
    return selectedModules;
}

void SelectionModel::deselectAllPins(){
    for (int i = 0; i < root->getModules()->size(); i++) {
        
        Module* m = root->getModules()->at(i);
        
        for (int j = 0; j < m->pins.size(); j++) {
            m->pins.at(j)->setSelected(false);
        }
    }
}
SelectionModel::State SelectionModel::checkForHitAndSelect(juce::Point<int> pos) {
    
    bool hit = false;
    
    for (int i = 0; i < root->getModules()->size(); i++) {
        if (root->getModules()->at(i)->getBounds().contains(pos)) {
   
                root->getModules()->at(i)->setSelected(true);
                getSelectedModules().push_back(root->getModules()->at(i));
                root->getModules()->at(i)->savePosition();
                hit = true;
                break;
            
        }
    }
    
    if (!hit) {
        clearSelection();
    }
    else {
        return MOVING_SELECTION;
    }
    
    return NONE;
}

void SelectionModel::select(juce::Point<int> pos) {
    for (int i = 0; i < root->getModules()->size(); i++) {
        if (root->getModules()->at(i)->getBounds().contains(pos)) {
            
            root->getModules()->at(i)->setSelected(true);
            getSelectedModules().push_back(root->getModules()->at(i));
            root->getModules()->at(i)->savePosition();
            break;
            
        }
    }
}

bool SelectionModel::isPointOnLineSegment(juce::Point<int> pt1, juce::Point<int> pt2, juce::Point<int> pt, double epsilon = 0.001)
{
    
    if (pt.x - std::max(pt1.x, pt2.x) > epsilon ||
        std::min(pt1.x, pt2.x) - pt.x > epsilon ||
        pt.y - std::max(pt1.y, pt2.y) > epsilon ||
        std::min(pt1.y, pt2.y) - pt.y > epsilon)
        return false;
    
    if (abs(pt2.x - pt1.x) < epsilon)
        return abs(pt1.x - pt.x) < epsilon ||abs(pt2.x - pt.x) < epsilon;
    if (abs(pt2.y - pt1.y) < epsilon)
        return abs(pt1.y - pt.y) < epsilon || abs(pt2.y - pt.y) < epsilon;
    
    double x = pt1.x + (pt.y - pt1.y) * (pt2.x - pt1.x) / (pt2.y - pt1.y);
    double y = pt1.y + (pt.x - pt1.x) * (pt2.y - pt1.y) / (pt2.x - pt1.x);
    
    return abs(pt.x - x) < epsilon ||abs(pt.y - y) < epsilon;
}

void SelectionModel::checkForConnection(juce::Point<int> pos) {
    for (int i = 0; i < root->getConnections()->size(); i++) {
        Connection* c = root->getConnections()->at(i);
        
        if (c->source != NULL && c->target != NULL)  {
            /*
            int x1 = c->source->getX() + c->a->x;
            int y1 = c->source->getY() + c->a->y + 5;
            int x2 = c->target->getX() + c->b->x;
            int y2 = c->target->getY() + c->b->y + 5;
            
            if (isPointOnLineSegment(Point<int>(x1, y1), Point<int>(x2, y2), pos, 5)) {
            */
            if(c->getPath().contains(pos.x,pos.y) ){
                
                c->selected = true;
            }
            else {
                c->selected = false;
            }
        }
        
    }
}

void SelectionModel::setRoot(Module *root) {
    this->root = root;
}
