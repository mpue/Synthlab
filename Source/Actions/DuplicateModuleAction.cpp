//
//  DuplicateModuleAction.cpp
//  Synthlab - App
//
//  Created by Matthias Pueski on 16.04.18.
//

#include "DuplicateModuleAction.h"
#include "../Module.h"
#include "../AudioOut.h"
#include "../AudioIn.h"
#include "../AuxOut.h"
#include "../PrefabFactory.h"
#include "../AudioManager.h"
#include "../ModuleUtils.h"

using juce::String;

DuplicateModuleAction::DuplicateModuleAction(SynthEditor* editor, juce::Point<int> position, Module* original) {
    this->editor = editor;
    this->position = position;
    this->moduleId = original->getId();
    this->module = original;
}

bool DuplicateModuleAction::undo() {
    
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

bool DuplicateModuleAction::perform() {
    

    
    Module* m = nullptr;
    
    if (module != nullptr) {
        
        if (module->isPrefab()) {
            m = PrefabFactory::getInstance()->getPrefab(moduleId, editor->getSampleRate(), editor->getBufferSize());
        
            module = m;
            
            AudioManager* p = AudioManager::getInstance();
            
            AudioOut* out;
            if ((out = dynamic_cast<AudioOut*>(m)) != NULL) {
                editor->getMixer()->getOutputChannels().push_back(out);
                String channelName = p->getOutputChannelNames().getReference(static_cast<int>(editor->getMixer()->getOutputChannels().size()) - 1);
                editor->addChannel(channelName, Mixer::Channel::Type::OUT);
                
                if (!editor->isRunning()) {
                    editor->setRunning(true);
                }
            }
            
            AuxOut* aux;
            if ((aux = dynamic_cast<AuxOut*>(m)) != NULL) {
                editor->getMixer()->getAuxChannels().push_back(aux);
                String channelName = "Aux "+ String(editor->getMixer()->getAuxChannels().size());
                editor->addChannel(channelName, Mixer::Channel::Type::AUX);
                if (!editor->isRunning()) {
                    editor->setRunning(true);
                }
            }
            
            AudioIn* in;
            if ((in = dynamic_cast<AudioIn*>(m)) != NULL) {
                editor->getMixer()->getInputChannels().push_back(in);
                String channelName = p->getInputChannelNames().getReference(static_cast<int>(editor->getMixer()->getInputChannels().size()) - 1);
                editor->addChannel(channelName, Mixer::Channel::Type::IN);
                if (!editor->isRunning()) {
                    editor->setRunning(true);
                }
            }
            
            editor->addChangeListener(m);
        }
        else {
            m = ModuleUtils::createCopy(this->module, editor);
        }
    }
    
    m->setTopLeftPosition(position);
    
    editor->addAndMakeVisible(m);
    editor->getModule()->getModules()->push_back(m);
    
    m->setSelected(true);
    m->savePosition();
    editor->getSelectionModel().getSelectedModules().push_back(m);
    editor->repaint();

    editor->setDirty(true);
    
    return true;
}


