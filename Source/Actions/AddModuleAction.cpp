//
//  AddModuleAction.cpp
//  Synthlab - App
//
//  Created by Matthias Pueski on 16.04.18.
//

#include "AddModuleAction.h"
#include "../Module.h"
#include "../AudioOut.h"
#include "../PrefabFactory.h"

AddModuleAction::AddModuleAction(SynthEditor* editor, Point<int> position, int moduleId) {
    this->editor = editor;
    this->position = position;
    this->moduleId = moduleId;
}

bool AddModuleAction::undo() {
    return true;
}

bool AddModuleAction::perform() {
    
    Module* m = PrefabFactory::getInstance()->getPrefab(moduleId, editor->getSampleRate(), editor->getBufferSize());
    editor->addChangeListener(m);
    m->setTopLeftPosition(position);
    
    editor->addAndMakeVisible(m);
    editor->getModule()->getModules()->push_back(m);
    
    m->setSelected(true);
    editor->getSelectedModules()->push_back(m);
    editor->repaint();
    
    AudioOut* out;
    if ((out = dynamic_cast<AudioOut*>(m)) != NULL) {
        editor->getOutputChannels().push_back(out);
        if (!editor->isRunning()) {
            editor->setRunning(true);
        }
    }
    return true;
}
