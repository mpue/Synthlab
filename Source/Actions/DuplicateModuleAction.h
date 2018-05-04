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

class DuplicateModuleAction : public juce::UndoableAction {
    
public:
    DuplicateModuleAction(SynthEditor* editor, juce::Point<int> position, Module* original);
    ~DuplicateModuleAction(){};
    
    virtual bool perform() override;
    virtual bool undo() override;
    
    void updateIndices(ValueTree &v);
    
private:
    
    SynthEditor* editor;
    juce::Point<int> position;
    int moduleId;
    Module* module = nullptr;
    
};


#endif /* AddModuleAction_h */

