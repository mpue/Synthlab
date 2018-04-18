//
//  AddConnectionAction.hpp
//  Synthlab - App
//
//  Created by Matthias Pueski on 18.04.18.
//

#ifndef AddConnectionAction_h
#define AddConnectionAction_h


#include "../JuceLibraryCode/JuceHeader.h"
#include "../SynthEditor.h"
#include "../Module.h"

class AddConnectionAction : public UndoableAction {
    
public:
    AddConnectionAction(SynthEditor* editor, Module* source);
    ~AddConnectionAction(){};
    
    virtual bool perform() override;
    virtual bool undo() override;
    
private:
    
    SynthEditor* editor;
    Point<int> position;
    Module* source;
    Module* root;
    
    Module* target = nullptr;
    Pin* a = nullptr;
    Pin* b = nullptr;
    Connection* c;
};


#endif /* AddConnectionAction_h*/
