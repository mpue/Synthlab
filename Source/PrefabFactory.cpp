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
#include "PadModule.h"
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
#include "ReverbModule.h"
#include "PitchBendModule.h"
#include "AudioRecorderModule.h"
#include "MidiClock.h"
#include "ImageModule.h"
#include "KeyboardModule.h"
#include "PublisherModule.h"
#include "SubscriberModule.h"
#include "ChorusModule.h"
#include "LedModule.h"
#include "AndModule.h"
#include "OrModule.h"
#include "NotModule.h"
#include "TriggerTimerModule.h"
#include "SliderModule.h"
#include "TextInputModule.h"
#include "CompareModule.h"
#include "TrackIn.h"

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
    else if (prefabs[id].getName() == "Midi clock") {
        m = new MidiClock();
        m->setName("Clock");
    }
    else if (prefabs[id].getName() == "Pad") {
        m = new PadModule();
        m->setName("Pad");
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
#if defined(JUCE_PLUGINHOST_AU) || defined(JUCE_PLUGINHOST_VST)
    else if (prefabs[id].getName() == "Plugin") {
        m = new PluginModule(sampleRate,bufferSize);
    }
#endif
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
    else if (prefabs[id].getName() == "Reverb") {
        m = new ReverbModule(sampleRate,bufferSize);
    }
    else if (prefabs[id].getName() == "Pitch bend") {
        m = new PitchBendModule();
    }
    else if (prefabs[id].getName() == "Audio recorder") {
        m = new AudioRecorderModule(sampleRate,bufferSize,AudioManager::getInstance()->getFormatManager());
    }
    else if (prefabs[id].getName() == "Image") {
        m = new ImageModule();
    }
    else if (prefabs[id].getName() == "Keyboard") {
        m = new KeyboardModule();
    }
    else if (prefabs[id].getName() == "Publisher") {
        m = new PublisherModule();
    }
    else if (prefabs[id].getName() == "Subscriber") {
        m = new SubscriberModule();
    }
    else if (prefabs[id].getName() == "Chorus") {
        m = new ChorusModule(sampleRate,bufferSize);
    }
    else if (prefabs[id].getName() == "LED") {
        m = new LedModule();
    }
    else if (prefabs[id].getName() == "And") {
        m = new AndModule();
    }
    else if (prefabs[id].getName() == "Or") {
        m = new OrModule();
    }
    else if (prefabs[id].getName() == "Not") {
        m = new NotModule();
    }
    else if (prefabs[id].getName() == "Trigger timer") {
        m = new TriggerTimerModule();
    }
    else if (prefabs[id].getName() == "Slider") {
        m = new SliderModule();
    }
    else if (prefabs[id].getName() == "TextInput") {
        m = new TextInputModule();
    }
    else if (prefabs[id].getName() == "Compare") {
        m = new CompareModule();
    }
    else if (prefabs[id].getName() == "Track In") {
        if (navigator != nullptr) {
            TrackIn* ti  = new TrackIn(navigator,sampleRate, bufferSize);
            Track* t = navigator->addTrack(Track::Type::AUDIO, 48000);
            t->setName("Audio "+ String(navigator->getTracks().size()+1));
            ti->setTrack(t);
            m = ti;
        }
    }
    
    if (m != nullptr) {
        m->setIndex(lastModuleIndex);
        m->setId(id);
        prefabs[id].addInstance();
        m->configurePins();
        numPrefabInstances++;
    
        // remember the last index in order to get a new one when creating an empty module
        lastModuleIndex++;
    }
    
    return m;
}

int PrefabFactory::getNumInstances(int id) {
    return prefabs[id].getNumInstances();
}

long PrefabFactory::getNextModuleIndex() {
    return lastModuleIndex++;
}

void PrefabFactory::reset() {
    for (std::map<int,Prefab>::iterator it = prefabs.begin();it != prefabs.end();) {
        (*it).second.reset();
        it++;
    }
    lastModuleIndex = 1;
}

void PrefabFactory::init(TrackNavigator* navigator) {
    this->navigator = navigator;
}