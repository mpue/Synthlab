//
//  PrefabFactory.cpp
//  Synthlab - App
//
//  Created by Matthias Pueski on 10.04.18.
//

#include "AudioManager.h"
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
#include "HPFilterModule.h"
#include "Knob.h"
#include "ADSRModule.h"
#include "SineModule.h"
#include "RandomModule.h"
#include "DelayModule.h"
#include "AddValueModule.h"
#include "DisplayValueModule.h"
#include "MultiplyModule.h"
#include "OscilloscopeModule.h"
#include "InverterModule.h"
#include "PitchToFrequencyModule.h"
#include "LabelModule.h"
#include "SamplerModule.h"
#include "AudioIn.h"
#include "AuxOut.h"
#include "DistortionModule.h"
#include "PluginModule.h"
#include "StepSequencerModule.h"
#include "OscillatorModule.h"
#include "AudioEngine/Pulse.h"
#include "MixerModule.h"
#include "TerminalModule.h"

using juce::Image;

PrefabFactory* PrefabFactory::instance = NULL;

Module* PrefabFactory::getPrefab(int id, float sampleRate, int bufferSize) {
    
    Module* m = nullptr;
    
    if (id == 0) {
        return m;
    }
    
    if (prefabs[id].getName() == "Add signal") {
        m = new AdderModule(bufferSize);
    }
    else if (prefabs[id].getName() == "Pitch to freq") {
        m = new PitchToFrequencyModule();
    }
    else if (prefabs[id].getName() == "Add value") {
        m = new AddValueModule();
    }
    else if (prefabs[id].getName() == "Multiply value") {
        m = new MultiplyModule();
    }
    else if (prefabs[id].getName() == "Invert value") {
        m = new InverterModule();
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
        Image* img = new Image(prefabImages[id]);
        m = new OscillatorModule<Sawtooth>(sampleRate,bufferSize, img);// SawtoothModule(sampleRate, bufferSize);
        m->setName("Sawtooth");
    }
    else if (prefabs[id].getName() == "Sine") {
        Image* img = new Image(prefabImages[id]);
        m = new OscillatorModule<Sine>(sampleRate,bufferSize, img);// SawtoothModule(sampleRate, bufferSize);
        m->setName("Sine");
    }
    else if (prefabs[id].getName() == "Pulse") {
        Image* img = new Image(prefabImages[id]);
        m = new OscillatorModule<Pulse>(sampleRate,bufferSize, img);// SawtoothModule(sampleRate, bufferSize);
        m->setName("Pulse");
    }
    else if (prefabs[id].getName() == "Random") {
        m = new RandomModule(sampleRate, bufferSize);
    }
    else if (prefabs[id].getName() == "White noise") {
        Image* img = new Image(prefabImages[id]);
        m = new OscillatorModule<WhiteNoise>(sampleRate,bufferSize, img);// SawtoothModule(sampleRate, bufferSize);
        m->setName("Noise");
    }
    else if (prefabs[id].getName() == "Audio out") {
        m = new AudioOut();
    }
    else if (prefabs[id].getName() == "Aux out") {
        m = new AuxOut();
    }
    else if (prefabs[id].getName() == "Audio in") {
        m = new AudioIn();
    }
    else if (prefabs[id].getName() == "Constant") {
        m = new Constant();
    }
    else if (prefabs[id].getName() == "Knob") {
        m = new Knob();
    }
    else if (prefabs[id].getName() == "Label") {
        m = new LabelModule("Label");
    }
    else if (prefabs[id].getName() == "LP filter") {
        m = new LPFilterModule(sampleRate,bufferSize);
    }
    else if (prefabs[id].getName() == "HP filter") {
        m = new HPFilterModule(sampleRate,bufferSize);
    }
    else if (prefabs[id].getName() == "ADSR") {
        m = new ADSRModule(sampleRate,bufferSize);
    }
    else if (prefabs[id].getName() == "Stereo delay") {
        m = new DelayModule(sampleRate,bufferSize);
    }
    else if (prefabs[id].getName() == "Display value") {
        m = new DisplayValueModule();
    }
    else if (prefabs[id].getName() == "Scope") {
        m = new OscilloscopeModule(sampleRate,bufferSize);
    }
    else if (prefabs[id].getName() == "Sampler") {
        m = new SamplerModule(sampleRate,bufferSize,AudioManager::getInstance()->getFormatManager());
    }
    else if (prefabs[id].getName() == "Distortion") {
        m = new DistortionModule(sampleRate,bufferSize);
    }
    else if (prefabs[id].getName() == "Plugin") {
        m = new PluginModule(sampleRate,bufferSize);
    }
    else if (prefabs[id].getName() == "Step sequencer") {
        m = new StepSequencerModule(sampleRate,bufferSize);
    }
    else if (prefabs[id].getName() == "Mixer") {
        m = new MixerModule(sampleRate,bufferSize);
    }
    else if (prefabs[id].getName() == "Terminal In") {
        m = new TerminalModule(TerminalModule::Direction::IN, TerminalModule::Type::VALUE);
    }
    else if (prefabs[id].getName() == "Terminal Out") {
        m = new TerminalModule(TerminalModule::Direction::OUT, TerminalModule::Type::VALUE);
    }
    
    m->setIndex(id + pow(10,prefabs[id].getNumInstances() + 1));
    m->setId(id);
    prefabs[id].addInstance();
    m->configurePins();
    numPrefabInstances++;
    return m;
}

int PrefabFactory::getNumInstances(int id) {
    return prefabs[id].getNumInstances();
}

void PrefabFactory::reset() {
    for (std::map<int,Prefab>::iterator it = prefabs.begin();it != prefabs.end();it++) {
        (*it).second.reset();
    }
}
