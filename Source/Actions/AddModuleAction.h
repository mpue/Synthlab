//
//  AddModuleAction.h
//  Synthlab - App
//
//  Created by Matthias Pueski on 16.04.18.
//

#ifndef AddModuleAction_h
#define AddModuleAction_h

#include "../JuceLibraryCode/JuceHeader.h"
#include "../Module.h"
#include "../SynthEditor.h"

class AddModuleAction : public juce::UndoableAction {
    
public:
    AddModuleAction(SynthEditor* editor, juce::Point<int> position, int moduleId);
    ~AddModuleAction(){};
    
    virtual bool perform() override;
    virtual bool undo() override;
    
private:
    
    SynthEditor* editor;
    juce::Point<int> position;
    int moduleId;
    Module* module = nullptr;
    
};


#endif /* AddModuleAction_h */
