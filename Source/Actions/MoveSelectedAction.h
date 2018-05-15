//
//  AddModuleAction.h
//  Synthlab - App
//
//  Created by Matthias Pueski on 16.04.18.
//

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "../Module.h"
#include "../SynthEditor.h"
#include "../ModuleUtils.h"

class MoveSelectedAction : public juce::UndoableAction {
    
public:
    MoveSelectedAction(SynthEditor* editor);
    ~MoveSelectedAction(){};
    
    virtual bool perform() override;
    virtual bool undo() override;
    
    void setDelta(int x, int y) {
        deltaX = x;
        deltaY = y;
    }
    
private:
    
    int deltaX = 0;
    int deltaY = 0;
    
    
    SynthEditor* editor;
    Module* module = nullptr;
    std::vector<Module*> movedModules;
    std::vector<Point<int>> oldPositions;
};




