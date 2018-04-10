//
//  PrefabFactory.cpp
//  Synthlab - App
//
//  Created by Matthias Pueski on 10.04.18.
//

#include "PrefabFactory.h"
#include "MidiGate.h"
#include "MidiOut.h"
#include "MidiNote.h"
#include "SawtoothModule.h"
#include "NoiseModule.h"
#include "AudioOut.h"
#include "Constant.h"
#include "AdderModule.h"

PrefabFactory* PrefabFactory::instance = NULL;

Module* PrefabFactory::getPrefab(String name, float sampleRate, int bufferSize) {

    Module* m = nullptr;
    
    if (name == "AdderModule") {
        m = new AdderModule();
        m->setIndex(Time::currentTimeMillis());
    }
    else if (name == "MidiGate") {
        m = new MidiGate();
        m->setName("Gate");
        m->setIndex(Time::currentTimeMillis());
    }
    else if (name == "MidiOut") {
        m = new MidiOut();
        m->setName("Midi Out");
        m->setIndex(Time::currentTimeMillis());
    }
    else if (name == "MidiNote") {
        m = new MidiNote();
        m->setName("Note");
        m->setIndex(Time::currentTimeMillis());
    }
    else if (name == "SawtoothModule") {
        m = new SawtoothModule(sampleRate, bufferSize);
        m->setIndex(Time::currentTimeMillis());
    }
    else if (name == "NoiseModule") {
        m = new NoiseModule(sampleRate, bufferSize);
        m->setIndex(Time::currentTimeMillis());
    }
    else if (name == "AudioOut") {
        m = new AudioOut();
        m->setIndex(Time::currentTimeMillis());
    }
    else if (name == "Constant") {
        m = new Constant();
        m->setIndex(Time::currentTimeMillis());
    }
    
        
    
    return m;
}
