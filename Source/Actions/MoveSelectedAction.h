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
#include "../Connection.h"

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

    // Verbindungen, deren Wegpunkte im selben Move mitverschoben wurden, plus
    // ihre Vor-Move-Wegpunkte fuer das Undo.
    std::vector<Connection*> movedConnections;
    std::vector<std::vector<Point<int>>> oldWaypoints;
};




