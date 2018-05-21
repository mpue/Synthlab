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
    
    SelectionModel();
    ~SelectionModel();
    
    Module* getSelectedModule();
    std::vector<Module*>& getSelectedModules();
    void clearSelection();
    void checkForPinSelection(juce::Point<int> position);
    void deselectAllPins();
    State checkForHitAndSelect(juce::Point<int> pos, bool additionalSelect);
    void checkForConnectionSelect(juce::Point<int> pos);
    void select(juce::Point<int> pos);
    void setRoot(Module* root);
    bool selectionContains(Module* m);
    void setCurrentLayer(int layer);
    
private:
    bool isPointOnLineSegment(juce::Point<int> pt1, juce::Point<int> pt2, juce::Point<int> pt, double epsilon);

    std::vector<Module*> selectedModules;
    Module* root;
    int layer = Module::Layer::ALL;
};

#endif /* SelectionModel_h */
