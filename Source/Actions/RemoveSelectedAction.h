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

    // An interface pin of the parent module that was paired with a removed
    // TerminalModule (pair convention: pin.index == terminal.index). Deleting
    // the terminal must also remove this pin, otherwise it lingers as an
    // orphaned terminal on the module's interface. Stashed here so undo() can
    // re-insert it at its original position.
    struct OrphanedPin {
        Pin*   pin;
        size_t pos;   // index into parent->getPins() before removal
    };

    SynthEditor* editor;
    std::vector<Connection*> removedConnections;
    std::vector<Module*>     removedModules;
    std::vector<PinLink>     removedPinLinks;
    std::vector<OrphanedPin> removedInterfacePins;
    bool modulesInEditor = true;
};




