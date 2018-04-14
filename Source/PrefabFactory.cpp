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
#include "Knob.h"
#include "ADSRModule.h"
#include "SineModule.h"
#include "RandomModule.h"
#include "DelayModule.h"

PrefabFactory* PrefabFactory::instance = NULL;

Module* PrefabFactory::getPrefab(int id, float sampleRate, int bufferSize) {

    Module* m = nullptr;
    
    if (prefabs[id].getName() == "Adder") {
        m = new AdderModule(bufferSize);
    }
    else if (prefabs[id].getName() == "Midi gate") {
        m = new MidiGate();
        m->setName("Gate");
    }
    else if (prefabs[id].getName() == "Midi out") {
        m = new MidiOut();
        m->setName("Midi Out");
    }
    else if (prefabs[id].getName() == "Midi note") {
        m = new MidiNote();
        m->setName("Note");
    }
    else if (prefabs[id].getName() == "Sawtooth") {
        m = new SawtoothModule(sampleRate, bufferSize);
    }
    else if (prefabs[id].getName() == "Sine") {
        m = new SineModule(sampleRate, bufferSize);
    }
    else if (prefabs[id].getName() == "Random") {
        m = new RandomModule(sampleRate, bufferSize);
    }
    else if (prefabs[id].getName() == "White noise") {
        m = new NoiseModule(sampleRate, bufferSize);
    }
    else if (prefabs[id].getName() == "Audio out") {
        m = new AudioOut();
    }
    else if (prefabs[id].getName() == "Constant") {
        m = new Constant();
    }
    else if (prefabs[id].getName() == "Knob") {
        m = new Knob();
    }
    else if (prefabs[id].getName() == "LP filter") {
        m = new LPFilterModule(sampleRate,bufferSize);
    }
    else if (prefabs[id].getName() == "ADSR") {
        m = new ADSRModule(sampleRate,bufferSize);
    }
    else if (prefabs[id].getName() == "Stereo delay") {
        m = new DelayModule(sampleRate,bufferSize);
    }
    
    m->setIndex(id + 100 * prefabs[id].getNumInstances());
    m->setId(id);
    prefabs[id].addInstance();
    m->configurePins();
    
    
    return m;
}
