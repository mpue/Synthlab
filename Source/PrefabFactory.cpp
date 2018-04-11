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
#include "LPFilterModule.h"

PrefabFactory* PrefabFactory::instance = NULL;

Module* PrefabFactory::getPrefab(int id, float sampleRate, int bufferSize) {

    Module* m = nullptr;
    
    if (prefabs[id] == "AdderModule") {
        m = new AdderModule();
    }
    else if (prefabs[id] == "MidiGate") {
        m = new MidiGate();
        m->setName("Gate");
    }
    else if (prefabs[id] == "MidiOut") {
        m = new MidiOut();
        m->setName("Midi Out");
    }
    else if (prefabs[id] == "MidiNote") {
        m = new MidiNote();
        m->setName("Note");
    }
    else if (prefabs[id] == "SawtoothModule") {
        m = new SawtoothModule(sampleRate, bufferSize);
    }
    else if (prefabs[id] == "NoiseModule") {
        m = new NoiseModule(sampleRate, bufferSize);
    }
    else if (prefabs[id] == "AudioOut") {
        m = new AudioOut();
    }
    else if (prefabs[id] == "Constant") {
        m = new Constant();
    }
    else if (prefabs[id] == "LP Filter") {
        m = new LPFilterModule(sampleRate,bufferSize);
    }
    
    m->setIndex(id + numPrefabInstances);
    m->setId(id);
    m->configurePins();
    
    numPrefabInstances++;
    
    return m;
}
