//
//  ModuleUtils.cpp
//  Synthlab - App
//
//  Created by Matthias Pueski on 04.05.18.
//  Copyright © 2018 Pueski. All rights reserved.
//

#include "ModuleUtils.h"
#include "Module.h"
#include "TerminalModule.h"
#include "Constant.h"
#include "Connection.h"
#include "PrefabFactory.h"
#include "AudioManager.h"
#include "Knob.h"
#include "LabelModule.h"
#include "Knob.h"
#include "MidiGate.h"
#include "ADSRModule.h"
#include "SineModule.h"
#include "SamplerModule.h"
#include "Monophonic.h"
#include "PluginModule.h"
#include "StepSequencerModule.h"
#include "AudioIn.h"
#include "AudioOut.h"
#include "AuxOut.h"
#include "MixerPanel.h"

void ModuleUtils::loadStructure(std::vector<Module *>* modules, std::vector<Connection*>* connections,juce::ValueTree *v, ChangeBroadcaster* broadcaster) {
    ValueTree mods = v->getChildWithName("Modules");
    
    for (int i = 0; i < mods.getNumChildren();i++) {
        ValueTree mod = mods.getChild(i);
        Module* m = loadModule(mod,broadcaster);
        configureModule(m,mod,broadcaster);
        modules->push_back(m);
        
        loadStructure(m->getModules(),m->getConnections(),&mod, broadcaster);
        ValueTree childCons = mod.getChildWithName("Connections");
        Logger::writeToLog("Module "+ m->getName() +" has "+String(childCons.getNumChildren())+ " connections.");
        if (childCons.getNumChildren() > 0)
            loadConnections(childCons, m->getModules(),m->getConnections());
        TerminalModule* t;
        
        if ((t = dynamic_cast<TerminalModule*>(m)) != nullptr) {
            t->setIndex(mod.getProperty("index").toString().getLargeIntValue());
        }
        connectTerminals(m);
    }
    
    ValueTree cons = v->getChildWithName("Connections");
    
    loadConnections(cons, modules, connections);
    
}

void ModuleUtils::loadConnections(juce::ValueTree &cons, std::vector<Module *>* modules, std::vector<Connection*>* connections) {
    for (int i = 0; i < cons.getNumChildren();i++) {
        ValueTree con = cons.getChild(i);
        
        Connection* c = new Connection();
        long sourceIndex = con.getProperty("source").toString().getLargeIntValue();
        long targetIndex = con.getProperty("target").toString().getLargeIntValue();
        long aIndex = con.getProperty("a").toString().getLargeIntValue();
        long bIndex = con.getProperty("b").toString().getLargeIntValue();
        
        Module* source = nullptr;
        Module* target = nullptr;
        Pin* a = nullptr;
        Pin* b = nullptr;
        
        for (int j = 0; j < modules->size(); j++) {
            
            Module* m = modules->at(j);
            
            if (m->getIndex() == sourceIndex) {
                source = m;
            }
            if (m->getIndex() == targetIndex) {
                target = m;
            }
            
            for (int k = 0; k < m->getPins().size();k++) {
                if (m->getPins().at(k)->index == aIndex) {
                    a = m->getPins().at(k);
                }
                if (m->getPins().at(k)->index == bIndex) {
                    b = m->getPins().at(k);
                }
            }
            
            if (source != nullptr && target != nullptr && a != nullptr && b != nullptr) {
                
                
                // EVENTS: in listens at out
                // OTHER: in gets its values from out
                if ((a->getType() != Pin::Type::EVENT && a->direction == Pin::Direction::IN && b->direction == Pin::Direction::OUT)
                    || (a->getType() == Pin::Type::EVENT && a->direction == Pin::Direction::OUT && b->direction == Pin::Direction::IN)) {
                    
                    if (!a->hasConnection(b)) {
                        
                        a->getConnections().push_back(b);
                        Logger::writeToLog("Connecting pin "+ String(a->index)+ " to pin "  +String(b->index));
                    }
                    
                    // xConnection* c = new Connection(source, a, target, b);
                    // root->getConnections()->push_back(c);
                    
                    c->a = a;
                    c->b = b;
                    c->source = source;
                    c->target = target;
                    if (!connectionExists(*connections, c)) {
                        Logger::writeToLog("Adding connection from module "+ source->getName() + ", index "+ String(source->getIndex()) + " to module "  +target->getName()+ ", index : " + String(target->getIndex()));
                        connections->push_back(c);
                    }
                    
                }
                else if ((a->getType() != Pin::Type::EVENT && a->direction == Pin::Direction::OUT && b->direction == Pin::Direction::IN)
                         || (a->getType() == Pin::Type::EVENT && a->direction == Pin::Direction::IN && b->direction == Pin::Direction::OUT)) {
                    if (!b->hasConnection(a)) {
                        b->getConnections().push_back(a);
                        Logger::writeToLog("Connecting pin "+ String(b->index)+ " to pin "  +String(a->index));
                    }
                    
                    // Connection* c = new Connection(target, b, source, a);
                    // root->getConnections()->push_back(c);
                    c->a = b;
                    c->b = a;
                    c->source = target;
                    c->target = source;
                    if (!connectionExists(*connections, c)) {
                        Logger::writeToLog("Adding connection from module "+ source->getName() + ", index "+ String(source->getIndex()) + " to module "  +target->getName()+ ", index : " + String(target->getIndex()));
                        connections->push_back(c);
                    }
                }
                break;
            }
            
        }
    }
}

Module* ModuleUtils::loadModule(ValueTree& mod, ChangeBroadcaster* broadcaster) {
    
    Module* m = nullptr;
    
    if (mod.getProperty("isPrefab").toString().getIntValue() == 1) {
        float sampleRate = AudioManager::getInstance()->getDeviceManager()->getCurrentAudioDevice()->getCurrentSampleRate();
        int bufferSize = AudioManager::getInstance()->getDeviceManager()->getCurrentAudioDevice()->getCurrentBufferSizeSamples();
        
        m = PrefabFactory::getInstance()->getPrefab(mod.getProperty("prefabId").toString().getIntValue(), sampleRate, bufferSize);
        broadcaster->addChangeListener(m);
        m->setIndex(mod.getProperty("index").toString().getLargeIntValue());
   
        LabelModule* label = dynamic_cast<LabelModule*>(m);
        
        if (label != nullptr) {
            label->setName(mod.getProperty("name"));
        }
        
        TerminalModule* t;
        
        if ((t = dynamic_cast<TerminalModule*>(m)) != NULL) {
            t->setName(mod.getProperty("name").toString());
        }
        
        ValueTree pins = mod.getChildWithName("Pins");
        
        for (int j = 0; j < pins.getNumChildren();j++) {
            ValueTree pin = pins.getChild(j);
            m->getPins().at(j)->index = pin.getProperty("index").toString().getLargeIntValue();
        }
    }
    else {
        m = new Module(mod.getProperty("name"));
        m->setIndex(mod.getProperty("index").toString().getLargeIntValue());
        broadcaster->addChangeListener(m);
        ValueTree pins = mod.getChildWithName("Pins");
        
        for (int j = 0; j < pins.getNumChildren();j++) {
            ValueTree pin = pins.getChild(j);
            Pin* p = new Pin(Pin::Type::AUDIO);
            
            juce::String name = pin.getProperty("name").toString();
            int type = pin.getProperty("type").toString().getIntValue();
            int direction = pin.getProperty("direction").toString().getIntValue();
            long index = pin.getProperty("index").toString().getLargeIntValue();
            int x = pin.getProperty("x").toString().getIntValue();
            int y = pin.getProperty("y").toString().getIntValue();
            
            Pin::Direction dir = static_cast<Pin::Direction>(direction);
            p->direction = dir;
            Pin::Type t = static_cast<Pin::Type>(type);
            p->setType(t);
            p->setName(name);
            p->index = index;
            p->x = x;
            p->y = y;
            m->addPin(p);
            
        }
        
    }
    
    m->setTopLeftPosition(mod.getProperty("x").toString().getIntValue(), mod.getProperty("y").toString().getIntValue());

    return m;
}

void ModuleUtils::connectTerminals(Module* m) {
    for (int i = 0; i < m->getModules()->size(); i++) {
        
        Module* module = m->getModules()->at(i);
        
        TerminalModule* t;
        
        if ((t = dynamic_cast<TerminalModule*>(module)) != NULL) {
            
            for (int j = 0; j < m->getPins().size();j++) {
                if (m->getPins().at(j)->index == t->getIndex()) {
                    Logger::writeToLog("Found terminal "+ String(t->getIndex()));
                    t->getPins().at(0)->setTerminal(m->getPins().at(j));
                    t->getPins().at(0)->setType(m->getPins().at(j)->getType());
                    m->getPins().at(j)->listeners.push_back(t);
                    t->addChangeListener(m->getPins().at(j));
                }
            }
            
        }
    }
}


bool ModuleUtils::connectionExists(std::vector<Connection*> connections,Connection *c) {
    
    for (int i = 0; i < connections.size();i++) {
        
        Connection* d = connections.at(i);
        if (d->a->index == c->a->index && d->b->index == c->b->index && d->source->getIndex() == c->source->getIndex() && d->target->getIndex() == c->target->getIndex()) {
            return true;
        }
        
    }
    
    return false;
}

void ModuleUtils::configureModule(Module *m, ValueTree& mod, ChangeBroadcaster* broadcaster) {

    
    Knob* k;
    
    if ((k = dynamic_cast<Knob*>(m)) != NULL) {
        k->setMaximum(mod.getProperty("maxvalue").toString().getFloatValue());
        k->setMinimum(mod.getProperty("minvalue").toString().getFloatValue());
        k->setStepSize(mod.getProperty("stepsize").toString().getFloatValue());
        k->setValue(mod.getProperty("value").toString().getFloatValue());
        k->setIsMidicontroller(mod.getProperty("isController").toString().getIntValue() > 0);
        k->setController(mod.getProperty("controllerNum").toString().getIntValue());
    }
    
    MidiGate* gate;
    
    if ((gate = dynamic_cast<MidiGate*>(m)) != NULL) {
        gate->setChannel(mod.getProperty("channel").toString().getIntValue());
    }
    
    ADSRModule* adsr;
    
    if ((adsr = dynamic_cast<ADSRModule*>(m)) != NULL) {
        broadcaster->addChangeListener(adsr);
        adsr->setAttack(mod.getProperty("attack").toString().getFloatValue());
        adsr->setDecay(mod.getProperty("decay").toString().getFloatValue());
        adsr->setSustain(mod.getProperty("sustain").toString().getFloatValue());
        adsr->setRelease(mod.getProperty("release").toString().getFloatValue());
        adsr->setMono(mod.getProperty("mono").toString().getIntValue() > 0);
    }
    
    Constant* c = nullptr;
    if ((c = dynamic_cast<Constant*>(m)) != NULL) {
        c->setValue(mod.getProperty("value").toString().getFloatValue());
    }
    
    SamplerModule* sm;
    
    if ((sm = dynamic_cast<SamplerModule*>(m)) != NULL) {
        
        for(int i = 0; i < mod.getNumChildren();i++) {
            if (mod.getChild(i).hasProperty("samplePath")) {
                
                int note = mod.getChild(i).getProperty("note").toString().getIntValue();
                String path = mod.getChild(i).getProperty("samplePath");
                sm->setSamplePath(path, note - 1);
                File file = File(path);
                if (file.exists()) {
                    FileInputStream* is = new FileInputStream(file);
                    sm->setSamplePath(file.getFullPathName(), note - 1);
                    sm->selectSample(note - 1);
                    sm->loadSample(is, note - 1);
                    
                    long start = mod.getChild(i).getProperty("sampleStart").toString().getLargeIntValue();
                    sm->getSamplerAt(note - 1)->setStartPosition(start);
                    long end = mod.getChild(i).getProperty("sampleEnd").toString().getLargeIntValue();
                    sm->getSamplerAt(note - 1)->setEndPosition(end);
                    // long length = mod.getChild(i).getProperty("sampleLength").toString().getLargeIntValue();
                    bool loop = mod.getChild(i).getProperty("loop").toString().getIntValue() > 0;
                    sm->getSamplerAt(note - 1)->setLoop(loop);
                    
                }
            }
        }
        
    }
    
    Monophonic* monophonic;
    
    if ((monophonic = dynamic_cast<Monophonic*>(m)) != NULL) {
        monophonic->setMono(mod.getProperty("mono").toString().getIntValue() > 0);
    }
    
    TerminalModule* t;
    
    if ((t = dynamic_cast<TerminalModule*>(m)) != NULL) {
        t->setType(static_cast<Terminal::Type>(mod.getProperty("type").toString().getIntValue()));
    }
    
    PluginModule* pm ;
    
    if ((pm = dynamic_cast<PluginModule*>(m)) != NULL) {
        pm->selectPlugin(mod.getProperty("plugin").toString());
        pm->setCurrentProgram(mod.getProperty("currentProgram").toString().getIntValue());
        pm->setPluginState(mod.getProperty("state").toString());
    }
    
    StepSequencerModule* ssm;
    
    if ((ssm = dynamic_cast<StepSequencerModule*>(m)) != NULL) {
        MemoryOutputStream* mos = new MemoryOutputStream();
        String dataString = mod.getProperty("config").toString();
        Base64::convertFromBase64(*mos, dataString);
        ssm->getEditor()->setConfiguration((uint8*)mos->getData());
        delete mos;
    }
    
    broadcaster->addChangeListener(m);
}

void ModuleUtils::saveStructure(std::vector<Module *>* modules, std::vector<Connection*>* connections, ValueTree* v) {
    ValueTree mods = ValueTree("Modules");
    
    for (std::vector<Module*>::iterator it = modules->begin(); it != modules->end(); ++it) {
        
        ValueTree file = ValueTree("Module");
        
        file.setProperty("name",(*it)->getName(), nullptr);
        file.setProperty("index",String((*it)->getIndex()), nullptr);
        file.setProperty("x",(*it)->getPosition().getX(),nullptr);
        file.setProperty("y",(*it)->getPosition().getY(),nullptr);
        file.setProperty("isPrefab",(*it)->isPrefab(),nullptr);
        file.setProperty("prefabId", (*it)->getId(), nullptr);
        
        Knob* k;
        
        if ((k = dynamic_cast<Knob*>((*it))) != NULL) {
            file.setProperty("minvalue",k->getMinimum(), nullptr);
            file.setProperty("maxvalue",k->getMaximum(), nullptr);
            file.setProperty("stepsize", k->getStepsize(), nullptr);
            file.setProperty("value", k->getValue(), nullptr);
            file.setProperty("isController",k->isMidiController(), nullptr);
            file.setProperty("controllerNum", k->getController(), nullptr);
        }
        
        ADSRModule* adsr;
        
        if ((adsr = dynamic_cast<ADSRModule*>((*it))) != NULL) {
            file.setProperty("attack",adsr->getAttack(), nullptr);
            file.setProperty("decay",adsr->getDecay(), nullptr);
            file.setProperty("sustain", adsr->getSustain(), nullptr);
            file.setProperty("release", adsr->getRelease(), nullptr);
            file.setProperty("mono", adsr->isMono(), nullptr);
        }
        
        Constant* c = nullptr;
        
        if ((c = dynamic_cast<Constant*>((*it))) != NULL) {
            file.setProperty("value", c->getValue(), nullptr);
        }
        
        MidiGate* gate = nullptr;
        
        if ((gate = dynamic_cast<MidiGate*>((*it))) != NULL) {
            file.setProperty("channel",gate->getChannel() , nullptr);
        }
        
        SamplerModule* sm;
        
        if ((sm = dynamic_cast<SamplerModule*>((*it))) != NULL) {
            
            for (int i = 0; i < 128; i++) {
                if (sm->hasSampleAt(i)) {
                    ValueTree v = ValueTree("sample");
                    v.setProperty("sampleStart",String(sm->getSamplerAt(i)->getStartPosition()),nullptr);
                    v.setProperty("sampleEnd",String(sm->getSamplerAt(i)->getEndPosition()), nullptr);
                    v.setProperty("sampleLength",String(sm->getSamplerAt(i)->getSampleLength()), nullptr);
                    v.setProperty("loop",sm->getSamplerAt(i)->isLoop(), nullptr);
                    v.setProperty("samplePath", sm->getSamplePath(i),nullptr);
                    v.setProperty("note", i + 1 , nullptr);
                    
                    file.addChild(v,-1,nullptr);
                }
            }
        }
        
        Monophonic* monophonic;
        
        if ((monophonic = dynamic_cast<Monophonic*>((*it))) != NULL) {
            file.setProperty("mono", monophonic->isMono(), nullptr);
        }
        
        TerminalModule* t;
        
        
        if ((t = dynamic_cast<TerminalModule*>((*it))) != NULL) {
            file.setProperty("type", t->getType(), nullptr);
        }
        
        PluginModule* pm;
        
        if ((pm = dynamic_cast<PluginModule*>((*it))) != NULL) {
            file.setProperty("plugin", pm->getPluginName(), nullptr);
            file.setProperty("currentProgram", pm->getCurrentProgram(), nullptr);
            file.setProperty("state", pm->getPluginState() ,nullptr);
        }
        
        StepSequencerModule* ssm;
        
        if ((ssm = dynamic_cast<StepSequencerModule*>((*it))) != NULL) {
            MemoryOutputStream* mos = new MemoryOutputStream();
            uint8* config = ssm->getEditor()->getConfiguration();
            Base64::convertToBase64(*mos,config , ssm->getEditor()->getConfigLength());
            mos->flush();
            file.setProperty("config", mos->toString(), nullptr);
            delete mos;
            delete config;
        }
        
        ValueTree pins = ValueTree("Pins");
        
        for (std::vector<Pin*>::iterator it2 =  (*it)->pins.begin(); it2 != (*it)->pins.end(); ++it2) {
            ValueTree pin = ValueTree("Pin");
            pin.setProperty("type", (*it2)->getType(), nullptr);
            pin.setProperty("name", (*it2)->getName(), nullptr);
            pin.setProperty("direction", (*it2)->direction, nullptr);
            pin.setProperty("index", String(((*it2)->index)), nullptr);
            pin.setProperty("x", (*it2)->x, nullptr);
            pin.setProperty("y", (*it2)->y, nullptr);
            pins.addChild(pin,-1,nullptr);
        }
        
        mods.addChild(file,-1, nullptr);
        file.addChild(pins,-1, nullptr);
        
        saveStructure((*it)->getModules(), (*it)->getConnections(), &file);
    }
    
    ValueTree cons = ValueTree("Connections");
    
    for (std::vector<Connection*>::iterator it = connections->begin(); it != connections->end(); ++it) {
        ValueTree con = ValueTree("Connection");
        con.setProperty("source", String((*it)->source->getIndex()), nullptr);
        con.setProperty("target", String((*it)->target->getIndex()), nullptr);
        con.setProperty("a", String((*it)->a->index), nullptr);
        con.setProperty("b", String((*it)->b->index), nullptr);
        cons.addChild(con,-1, nullptr);
    }
    
    v->addChild(mods, -1, nullptr);
    v->addChild(cons, -1, nullptr);
    
}

void ModuleUtils::updateIndices(juce::ValueTree &v, int offset) {
     for(int i = 0; i < v.getNumChildren();i++) {
         if (v.getChild(i).hasProperty("index")) {
             v.getChild(i).setProperty("index", v.getChild(i).getProperty("index").toString().getLargeIntValue()+offset,nullptr);
         }
         if (v.getChild(i).hasProperty("source")) {
             v.getChild(i).setProperty("source", v.getChild(i).getProperty("source").toString().getLargeIntValue()+offset,nullptr);
         }
         if (v.getChild(i).hasProperty("target")) {
             v.getChild(i).setProperty("target", v.getChild(i).getProperty("target").toString().getLargeIntValue()+offset,nullptr);
         }
         if (v.getChild(i).hasProperty("a")) {
             v.getChild(i).setProperty("a", v.getChild(i).getProperty("a").toString().getLargeIntValue()+offset,nullptr);
         }
         if (v.getChild(i).hasProperty("b")) {
             v.getChild(i).setProperty("b", v.getChild(i).getProperty("b").toString().getLargeIntValue()+offset,nullptr);
         }
         ValueTree child = v.getChild(i);
         updateIndices(child, offset);
      }
      
}

Module* ModuleUtils::createCopy(Module *original, ChangeBroadcaster* broadcaster) {
    
    Module* m = nullptr;
    
    ValueTree cloneTree = ValueTree("Module");
    m = new Module(original->getName());
    saveStructure(original->getModules(),original->getConnections(), &cloneTree);
    savePins(original, cloneTree);
        
    
    
    Logger::writeToLog(cloneTree.toXmlString());
    updateIndices(cloneTree, Time::getMillisecondCounter());
    Logger::writeToLog(cloneTree.toXmlString());
    loadPins(m, cloneTree);
    loadStructure(m->getModules(), m->getConnections(),&cloneTree, broadcaster);

    m->resized();
    
    ModuleUtils::connectTerminals(m);
    
    return m;
}

void ModuleUtils::savePins(Module* m, ValueTree& v) {
    
    ValueTree pins = ValueTree("Pins");
    
    for (std::vector<Pin*>::iterator it2 =  m->pins.begin(); it2 != m->pins.end(); ++it2) {
        ValueTree pin = ValueTree("Pin");
        pin.setProperty("type", (*it2)->getType(), nullptr);
        pin.setProperty("name", (*it2)->getName(), nullptr);
        pin.setProperty("direction", (*it2)->direction, nullptr);
        pin.setProperty("index", String(((*it2)->index)), nullptr);
        pin.setProperty("x", (*it2)->x, nullptr);
        pin.setProperty("y", (*it2)->y, nullptr);
        pins.addChild(pin,-1,nullptr);
    }
    
    v.addChild(pins, -1, nullptr);
}


void ModuleUtils::loadPins(Module* m,ValueTree& mod) {
    ValueTree pins = mod.getChildWithName("Pins");
    
    for (int j = 0; j < pins.getNumChildren();j++) {
        ValueTree pin = pins.getChild(j);
        Pin* p = new Pin(Pin::Type::AUDIO);
        
        juce::String name = pin.getProperty("name").toString();
        int type = pin.getProperty("type").toString().getIntValue();
        int direction = pin.getProperty("direction").toString().getIntValue();
        long index = pin.getProperty("index").toString().getLargeIntValue();
        int x = pin.getProperty("x").toString().getIntValue();
        int y = pin.getProperty("y").toString().getIntValue();
        
        Pin::Direction dir = static_cast<Pin::Direction>(direction);
        p->direction = dir;
        Pin::Type t = static_cast<Pin::Type>(type);
        p->setType(t);
        p->index = index;
        p->setName("name");
        p->x = x;
        p->y = y;
        m->addPin(p);
        
    }
}

void ModuleUtils::removeModule(Module *root,Module* module,MixerPanel* mixer, ChangeBroadcaster* broadcaster) {
    
    
    AudioOut* out = nullptr;
    
    if ((out = dynamic_cast<AudioOut*>(module)) != nullptr){
        mixer->removeChannel(out->getChannelIndex());
        for(std::vector<AudioOut*>::iterator it = mixer->getOutputChannels().begin();it != mixer->getOutputChannels().end();) {
            if ((*it)->getChannelIndex() == out->getChannelIndex()) {
                it = mixer->getOutputChannels().erase(it);
            }
            else {
                ++it;
            }
        }
    }
    
    
    AudioIn* in = nullptr;
    
    if ((in = dynamic_cast<AudioIn*>(module)) != nullptr){
        mixer->removeChannel(in->getChannelIndex());
        for(std::vector<AudioIn*>::iterator it = mixer->getInputChannels().begin();it != mixer->getInputChannels().end();) {
            if ((*it)->getChannelIndex() == in->getChannelIndex()) {
                it = mixer->getInputChannels().erase(it);
            }
            else {
                ++it;
            }
        }
    }
    
    AuxOut* aux = nullptr;
    
    if ((aux = dynamic_cast<AuxOut*>(module)) != nullptr){
        mixer->removeChannel(aux->getChannelIndex());
        for(std::vector<AuxOut*>::iterator it = mixer->getAuxChannels().begin();it != mixer->getAuxChannels().end();) {
            if ((*it)->getChannelIndex() == aux->getChannelIndex()) {
                it = mixer->getAuxChannels().erase(it);
            }
            else {
                ++it;
            }
        }
    }
    
    
    vector<long> pinsToBeRemoved;
    
    broadcaster->removeChangeListener(module);
    
    // find the indices of the pins being involved in the disconnect
    
    for (int j = 0; j < root->getModules()->size(); j++) {
        if (root->getModules()->at(j)->getIndex() != module->getIndex()) {
            
            for (int k = 0; k < root->getModules()->at(j)->getPins().size(); k++) {
                
                // for every connection of each pin
                for (int l = 0; l < root->getModules()->at(j)->getPins().at(k)->getConnections().size(); l++) {
                    
                    // for each pin of the module being removed
                    for (int n = 0; n < module->getPins().size();n++) {
                        // if the index matches remove pin from vector
                        
                        if (root->getModules()->at(j)->getPins().at(k)->getConnections().at(l)->index == module->getPins().at(n)->index) {
                            pinsToBeRemoved.push_back(root->getModules()->at(j)->getPins().at(k)->getConnections().at(l)->index);
                            
                        }
                    }
                    
                }
            }
        }
    }
    
    // remove according pins
    
    for (int i = 0; i < pinsToBeRemoved.size();i++) {
        for (int j = 0; j < root->getModules()->size(); j++) {
            
            for (int k = 0; k < root->getModules()->at(j)->getPins().size();k++) {
                
                Pin* p = root->getModules()->at(j)->getPins().at(k);
                for (std::vector<Pin*>::iterator it = p->getConnections().begin();it != p->getConnections().end();) {
                    if ((*it)->index == pinsToBeRemoved.at(i)) {
                        it = p->getConnections().erase(it);
                    }
                    else {
                        ++it;
                    }
                }
            }
        }
    }
    
    // remove dangling connections from other modules
    
    std::vector<Connection*>* cons = root->getConnections();
    
    for(int i = 0;i <root->getModules()->size();i++) {
        for (std::vector<Connection*>::iterator it = cons->begin(); it != cons->end(); ) {
            if ((*it)->source->getIndex() == module->getIndex() ||
                (*it)->target->getIndex() == module->getIndex() ) {
                delete (*it);
                it = cons->erase(it);
            }
            else {
                ++it;
            }
        }
    }
    
    
}

bool ModuleUtils::connectModules(Module* source, Module* target, int pin) {
    
    Pin* output = nullptr;
    Terminal::Type inputType = target->getPins().at(pin)->getType();
    
    switch (inputType) {
        case Terminal::VALUE:
            return connectModuleValues(source, target, pin);
            break;
        case Terminal::EVENT:
            return connectModuleEvents(source, target, pin);
            break;
        case Terminal::AUDIO:
            return connectModuleAutio(source, target, pin);
            break;
            
        default:
            return false;
            break;
    }
    return false;
}

bool ModuleUtils::connectModuleValues(Module* source, Module* target, int inputPin)
{
    if(Terminal::Direction::IN != target->getPins().at(inputPin)->getDirection())
    {
        return false;
    }
    
    Pin* sourePin = source->getValueOutputPin();
    
    if(nullptr != sourePin)
    {
        target->getPins().at(inputPin)->getConnections().push_back(sourePin);
        return true;
    }
        
    return false;
}

bool ModuleUtils::connectModuleAutio(Module* source, Module* target, int pin)
{
    return false;
}

bool ModuleUtils::connectModuleEvents(Module* source, Module* target, int pin)
{
    Pin* subscriber = target->getPins().at(pin);
    if(Terminal::Direction::IN != subscriber->getDirection())
    {
        return false;
    }
    
    for (int i = 0; i < source->getPins().size();i++)
    {
        Pin* publisher = source->getPins().at(i);
        
        if(Terminal::EVENT == publisher->getType() && Terminal::Direction::OUT == publisher->getDirection() )
        {
            publisher->getConnections().push_back(subscriber);
            return true;
        }
    }
    
    return false;
}
