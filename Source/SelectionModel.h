//
//  SelectionModel.hpp
//  Synthlab - App
//
//  Created by Matthias Pueski on 20.04.18.
//

#ifndef SelectionModel_h
#define SelectionModel_h

#include "Module.h"
#include "../JuceLibraryCode/JuceHeader.h"

class SelectionModel {
    
public:
    
    enum State {
        NONE,
        DRAGGING_SELECTION,
        SELECT_ELEMENT,
        MOVING_SELECTION,
        DRAGGING_CONNECTION
    };
    
    SelectionModel(Module* root);
    ~SelectionModel();
    
    Module* getSelectedModule();
    std::vector<Module*>* getSelectedModules();
    void clearSelection();
    void checkForPinSelection(Point<int> position);
    void deselectAllPins();
    State checkForHitAndSelect(Point<int> pos);
    void checkForConnection(Point<int> pos);
    void select(Point<int> pos);
    
private:
    bool isPointOnLineSegment(Point<int> pt1, Point<int> pt2, Point<int> pt, double epsilon);

    std::vector<Module*>* selectedModules;
    Module* root;
};

#endif /* SelectionModel_h */
