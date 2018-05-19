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
#include "../ImageModule.h"

using juce::String;

AddModuleAction::AddModuleAction(SynthEditor* editor, juce::Point<int> position, int moduleId) {
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
    
    AudioManager* audioManager = AudioManager::getInstance();
    AudioOut* out;
    
    if ((out = dynamic_cast<AudioOut*>(m)) != NULL) {
        int numCurrentOutputs = static_cast<int>(editor->getMixer()->getInputChannels().size()) * 2;
        // take care! we have N input channels but from the audio interfaces point of view we have twice as much channels!
        
        bool useDefaultOutput = false;
        
        if (audioManager->getOutputChannelNames().size() == 2) {
            String channelName = audioManager->getOutputChannelNames().getReference(0);
            useDefaultOutput = true;
        }
        
        if (numCurrentOutputs < audioManager->getNumActiveInputs() || (useDefaultOutput && numCurrentOutputs == 0)) {
            editor->getMixer()->getOutputChannels().push_back(out);
            String channelName = audioManager->getOutputChannelNames().getReference(static_cast<int>(editor->getMixer()->getOutputChannels().size()) - 1);
            int channelIndex = editor->addChannel(channelName, Mixer::Channel::Type::OUT);
            out->setChannelIndex(channelIndex);
            m->setName(channelName);
            editor->getMixer()->setVolume(channelIndex, out->getVolume());
            editor->getMixer()->setModule(channelIndex, out);
            if (!editor->isRunning()) {
                editor->setRunning(true);
            }
        }
        else {
            delete m;
            return false;
        }
        
        

    }

    
    AuxOut* aux;
    if ((aux = dynamic_cast<AuxOut*>(m)) != NULL) {
        editor->getMixer()->getAuxChannels().push_back(aux);
        String channelName = "Aux "+ String(editor->getMixer()->getAuxChannels().size());
        int channelIndex = editor->addChannel(channelName, Mixer::Channel::Type::AUX);
        aux->setChannelIndex(channelIndex);
        m->setName(channelName);
        editor->getMixer()->setVolume(channelIndex, aux->getVolume());
        editor->getMixer()->setModule(channelIndex, aux);
        if (!editor->isRunning()) {
            editor->setRunning(true);
        }
    }
    
    AudioIn* in;
    
    if ((in = dynamic_cast<AudioIn*>(m)) != NULL) {
        
        int numCurrentInputs = static_cast<int>(editor->getMixer()->getInputChannels().size()) * 2; // take care! we have N input channels but from the audio interfaces point of view we have twice as much channels!
        
        if (numCurrentInputs < audioManager->getNumActiveInputs()) {
        
            editor->getMixer()->getInputChannels().push_back(in);
            String channelName = audioManager->getInputChannelNames().getReference(static_cast<int>(editor->getMixer()->getInputChannels().size()) - 1);
            m->setName(channelName);
            int channelIndex = editor->addChannel(channelName, Mixer::Channel::Type::IN);
            in->setChannelIndex(channelIndex);
            editor->getMixer()->setVolume(channelIndex, in->getVolume());
            editor->getMixer()->setModule(channelIndex, in);
            if (!editor->isRunning()) {
                editor->setRunning(true);
            }
        }
        else {
            delete m;
            return false;
        }
    }
    
    TerminalModule* t;
    
    if ((t = dynamic_cast<TerminalModule*>(m)) != NULL) {
        Pin* p = new Pin(Pin::Type::VALUE);
        t->getPins().at(0)->setTerminal(p);
        p->listeners.push_back(t);
        p->setName(t->getName());
        t->addChangeListener(p);
        if (t->getDirection() == TerminalModule::Direction::IN) {
            editor->getModule()->addPin(Pin::Direction::IN, p);
        }
        else {
            editor->getModule()->addPin(Pin::Direction::OUT, p);
        }
        t->setIndex(p->index);
        
        editor->getModule()->repaint();
        
    }
    

    editor->addChangeListener(m);
    
    m->setTopLeftPosition(position);
    m->savePosition();
    m->setUiPosition(position.x, position.y);
    m->saveUiPosition();
    
    editor->addAndMakeVisible(m);
    editor->getModule()->getModules()->push_back(m);
    
    ImageModule* im;
    
    if ((im = dynamic_cast<ImageModule*>(m)) != NULL) {
        im->toBack();   
    }
    
    m->setSelected(true);

    editor->getSelectionModel().getSelectedModules().push_back(m);
    editor->repaint();
    
    module = m;
    
    editor->setDirty(true);
    
    return true;
}
