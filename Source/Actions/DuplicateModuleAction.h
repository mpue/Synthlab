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

class DuplicateModuleAction : public juce::UndoableAction {
    
public:
    DuplicateModuleAction(SynthEditor* editor, juce::Point<int> position, Module* original);
    ~DuplicateModuleAction(){};
    
    virtual bool perform() override;
    virtual bool undo() override;

    
private:
    
    SynthEditor* editor;
    juce::Point<int> position;
    int moduleId;
    Module* module = nullptr;
    
};




