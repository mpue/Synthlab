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

class RemoveSelectedAction : public juce::UndoableAction {

public:
    RemoveSelectedAction(SynthEditor* editor);
    ~RemoveSelectedAction();

    virtual bool perform() override;
    virtual bool undo() override;
    void removeSelectedItem();

private:

    struct PinLink {
        Pin* outer;   // pin on a module that stays in the editor
        Pin* inner;   // pin on a removed module
    };

    SynthEditor* editor;
    std::vector<Connection*> removedConnections;
    std::vector<Module*>     removedModules;
    std::vector<PinLink>     removedPinLinks;
    bool modulesInEditor = true;
};




