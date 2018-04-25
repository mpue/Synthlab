//
//  AddModuleAction.cpp
//  Synthlab - App
//
//  Created by Matthias Pueski on 16.04.18.
//

#include "AddModuleAction.h"
#include "../Module.h"
#include "../AudioOut.h"
#include "../AudioIn.h"
#include "../AuxOut.h"
#include "../PrefabFactory.h"
#include "../AudioManager.h"

AddModuleAction::AddModuleAction(SynthEditor* editor, Point<int> position, int moduleId) {
    this->editor = editor;
    this->position = position;
    this->moduleId = moduleId;
}

bool AddModuleAction::undo() {
    
    if (module == nullptr) {
        return false;
    }
    
    for (std::vector<Module*>::iterator it = editor->getModule()->getModules()->begin();it != editor->getModule()->getModules()->end();) {
        
        if ((*it)->getIndex() == module->getIndex()) {
            editor->removeChangeListener((*it));
            // delete (*it);
            it = editor->getModule()->getModules()->erase(it);
            editor->removeChildComponent((*it));
        }
        else {
            ++it;
        }
    }
    
    editor->repaint();
    
    
    return true;
}

bool AddModuleAction::perform() {
    
    Module* m = module;
    
    if (m == nullptr) {
        m = PrefabFactory::getInstance()->getPrefab(moduleId, editor->getSampleRate(), editor->getBufferSize());
    }
    
    editor->addChangeListener(m);
    m->setTopLeftPosition(position);
    
    editor->addAndMakeVisible(m);
    editor->getModule()->getModules()->push_back(m);
    
    m->setSelected(true);
    m->savePosition();
    editor->getSelectionModel()->getSelectedModules()->push_back(m);
    editor->repaint();
    
    module = m;
    
    AudioManager* p = AudioManager::getInstance();
    
    AudioOut* out;
    if ((out = dynamic_cast<AudioOut*>(m)) != NULL) {
        editor->getOutputChannels().push_back(out);
        String channelName = p->getOutputChannelNames().getReference(editor->getOutputChannels().size() - 1);
        editor->addChannel(channelName, Mixer::Channel::Type::OUT);
        
        if (!editor->isRunning()) {
            editor->setRunning(true);
        }
    }
    
    AuxOut* aux;
    if ((aux = dynamic_cast<AuxOut*>(m)) != NULL) {
        editor->getAuxChannels().push_back(aux);
        String channelName = "Aux "+ String(editor->getAuxChannels().size());
        editor->addChannel(channelName, Mixer::Channel::Type::AUX);
        if (!editor->isRunning()) {
            editor->setRunning(true);
        }
    }
    
    AudioIn* in;
    if ((in = dynamic_cast<AudioIn*>(m)) != NULL) {
        editor->getInputChannels().push_back(in);
        String channelName = p->getInputChannelNames().getReference(editor->getInputChannels().size() - 1);
        editor->addChannel(channelName, Mixer::Channel::Type::IN);
        if (!editor->isRunning()) {
            editor->setRunning(true);
        }
    }
    
    return true;
}
