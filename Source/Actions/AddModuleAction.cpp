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
#include "../TerminalModule.h"

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
    
    if (moduleId < 1) {
        return false;
    }
    
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
    editor->getSelectionModel().getSelectedModules().push_back(m);
    editor->repaint();
    
    module = m;
    
    AudioManager* p = AudioManager::getInstance();
    
    AudioOut* out;
    if ((out = dynamic_cast<AudioOut*>(m)) != NULL) {
        editor->getOutputChannels().push_back(out);
        String channelName = p->getOutputChannelNames().getReference(static_cast<int>(editor->getOutputChannels().size()) - 1);
        int channelIndex = editor->addChannel(channelName, Mixer::Channel::Type::OUT);
        out->setChannelIndex(channelIndex);
        if (!editor->isRunning()) {
            editor->setRunning(true);
        }
    }
    
    AuxOut* aux;
    if ((aux = dynamic_cast<AuxOut*>(m)) != NULL) {
        editor->getAuxChannels().push_back(aux);
        String channelName = "Aux "+ String(editor->getAuxChannels().size());
        int channelIndex = editor->addChannel(channelName, Mixer::Channel::Type::AUX);
        aux->setChannelIndex(channelIndex);
        if (!editor->isRunning()) {
            editor->setRunning(true);
        }
    }
    
    AudioIn* in;
    if ((in = dynamic_cast<AudioIn*>(m)) != NULL) {
        editor->getInputChannels().push_back(in);
        String channelName = p->getInputChannelNames().getReference(static_cast<int>(editor->getInputChannels().size()) - 1);
        int channelIndex = editor->addChannel(channelName, Mixer::Channel::Type::IN);
        in->setChannelIndex(channelIndex);
        if (!editor->isRunning()) {
            editor->setRunning(true);
        }
    }
    
    TerminalModule* t;
    if ((t = dynamic_cast<TerminalModule*>(m)) != NULL) {
        Pin* p = new Pin(Pin::Type::VALUE);
        t->getPins().at(0)->setTerminal(p);
       
        if (t->getDirection() == TerminalModule::Direction::IN) {
            editor->getModule()->addPin(Pin::Direction::IN, p);
        }
        else {
            editor->getModule()->addPin(Pin::Direction::OUT, p);
        }
        t->setIndex(p->index);
        
        editor->getModule()->repaint();
        
    }
    
    return true;
}
