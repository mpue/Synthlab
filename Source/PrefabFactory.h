//
//  PrefabFactory.hpp
//  Synthlab - App
//
//  Created by Matthias Pueski on 10.04.18.
//

#ifndef PrefabFactory_hpp
#define PrefabFactory_hpp

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"
#include <map>

class PrefabFactory {
    
public:
    
    class Prefab {
    public:
        Prefab() {};
        Prefab(juce::String name, juce::String category){
            this->name = name;
            this->category = category;
            this->icon = icon;
        };
        juce::String getName() {
            return name;
        }
        juce::String getCategory() {
            return category;
        }
        int getId() {
            return prefabId;
        }
        
        void setId(int prefabId) {
            this->prefabId = prefabId;
        }
        
        juce::Image* getIcon() {
            return &icon;
        }
        
        void setIcon(juce::Image& icon) {
            this->icon = icon;
        }
        
        int getNumInstances() {
            return numInstances;
        }
        
        void addInstance() {
            numInstances++;
        }
        void reset() {
            numInstances = 0;
        }
        
    private:
        int numInstances = 0;
        juce::String name;
        juce::String category;
        juce::Image icon;
        int prefabId;
    };
    
    static PrefabFactory* getInstance() {
        if (instance == NULL) {
            instance = new PrefabFactory();
        }
        return instance;
    }
    
    static void destroy() {
        delete instance;
    }
    
    
    juce::Image getImage(int prefabId) {
        return prefabImages[prefabId];
    }
    
    juce::PopupMenu createPrefabMenu();
    Module* getPrefab(int id, float sampleRate, int bufferSize);
    std::map<int,Prefab> getPrefabNames() {return prefabs;};
    juce::StringArray* getCategories() { return categories;};
    int getNumInstances(int id);
    void reset();
private:
    static PrefabFactory* instance;
    
    std::map<int,Prefab> prefabs;
    std::map<int,juce::Image> prefabImages;
    
    juce::StringArray* categories;
    
    PrefabFactory() {
        prefabs[53] = Prefab("Sampler","Sound sources");
        prefabs[54] = Prefab("Label","Controls");
        prefabs[55] = Prefab("Pitch to freq","Math");
        prefabs[56] = Prefab("Invert value","Math");
        prefabs[57] = Prefab("Multiply value","Math");
        prefabs[58] = Prefab("Display value","Debug");
        prefabs[59] = Prefab("Add value","Math");
        prefabs[60] = Prefab("Add signal","Math");
        prefabs[61] = Prefab("Midi gate","MIDI");
        prefabs[62] = Prefab("Midi out","MIDI");
        prefabs[63] = Prefab("Midi note","MIDI");
        prefabs[64] = Prefab("Sawtooth","Sound sources");
        prefabs[65] = Prefab("White noise","Sound sources");
        prefabs[66] = Prefab("Audio out","Input / Output");
        prefabs[67] = Prefab("Constant","Math");
        prefabs[68] = Prefab("LP filter","Filter");
        prefabs[69] = Prefab("Knob","Controls");
        prefabs[70] = Prefab("ADSR","Input / Output");
        prefabs[71] = Prefab("Sine","Sound sources");
        prefabs[72] = Prefab("Random","Sound sources");
        prefabs[73] = Prefab("Stereo delay","FX");
        prefabs[74] = Prefab("HP filter","Filter");
        prefabs[75] = Prefab("Scope","Debug");
        prefabs[76] = Prefab("Audio in","Input / Output");
        prefabs[77] = Prefab("Aux out","Input / Output");
        prefabs[78] = Prefab("Distortion","FX");
        prefabs[79] = Prefab("Plugin","FX");    
        prefabs[80] = Prefab("Step sequencer","MIDI");
        prefabs[81] = Prefab("Pulse","Sound sources");
        prefabs[82] = Prefab("Mixer","Audio");
        prefabs[83] = Prefab("Terminal In","Input / Output");
        prefabs[84] = Prefab("Terminal Out","Input / Output");
        prefabs[85] = Prefab("Reverb","FX");
        prefabs[86] = Prefab("Pitch bend","MIDI");
        prefabs[87] = Prefab("Audio recorder","Audio");
        prefabs[88] = Prefab("Pad","Controls");
        prefabs[89] = Prefab("Midi clock","MIDI");
        prefabs[90] = Prefab("Image","Controls");
        prefabs[91] = Prefab("Keyboard","Controls");
        prefabs[92] = Prefab("Publisher","Controls");
        prefabs[93] = Prefab("Subscriber","Controls");
        prefabs[94] = Prefab("Chorus","FX");
        prefabs[95] = Prefab("LED","Controls");
        prefabs[96] = Prefab("And","Logic");
        prefabs[97] = Prefab("Or","Logic");
        prefabs[98] = Prefab("Not","Logic");
        prefabs[52] = Prefab("Trigger timer","MIDI");
        prefabs[53] = Prefab("Slider","Controls");
        
        prefabImages[53] = juce::ImageCache::getFromMemory(BinaryData::sampler_png, BinaryData::sampler_pngSize);
        prefabImages[54] = juce::ImageCache::getFromMemory(BinaryData::module_png, BinaryData::module_pngSize);
        prefabImages[55] = juce::ImageCache::getFromMemory(BinaryData::p2f_png, BinaryData::p2f_pngSize);
        prefabImages[56] = juce::ImageCache::getFromMemory(BinaryData::inverter_png, BinaryData::inverter_pngSize);
        prefabImages[57] = juce::ImageCache::getFromMemory(BinaryData::multiplier_png, BinaryData::multiplier_pngSize);
        prefabImages[58] = juce::ImageCache::getFromMemory(BinaryData::display_png, BinaryData::display_pngSize);
        prefabImages[59] = juce::ImageCache::getFromMemory(BinaryData::adder_png, BinaryData::adder_pngSize);
        prefabImages[60] = juce::ImageCache::getFromMemory(BinaryData::adder_png, BinaryData::adder_pngSize);
        prefabImages[61] = juce::ImageCache::getFromMemory(BinaryData::midiin_png, BinaryData::midiin_pngSize);
        prefabImages[62] = juce::ImageCache::getFromMemory(BinaryData::midiout_png, BinaryData::midiout_pngSize);
        prefabImages[63] = juce::ImageCache::getFromMemory(BinaryData::note_png, BinaryData::note_pngSize);
        prefabImages[64] = juce::ImageCache::getFromMemory(BinaryData::saw_png, BinaryData::saw_pngSize);
        prefabImages[65] = juce::ImageCache::getFromMemory(BinaryData::whitenoise_png, BinaryData::whitenoise_pngSize);
        prefabImages[66] = juce::ImageCache::getFromMemory(BinaryData::speaker_png, BinaryData::speaker_pngSize);
        prefabImages[67] = juce::ImageCache::getFromMemory(BinaryData::module_png, BinaryData::module_pngSize);
        prefabImages[68] = juce::ImageCache::getFromMemory(BinaryData::lp_png, BinaryData::lp_pngSize);
        prefabImages[69] = juce::ImageCache::getFromMemory(BinaryData::knob_png, BinaryData::knob_pngSize);
        prefabImages[70] = juce::ImageCache::getFromMemory(BinaryData::adsr_png, BinaryData::adder_pngSize);
        prefabImages[71] = juce::ImageCache::getFromMemory(BinaryData::sine_png, BinaryData::sine_pngSize);
        prefabImages[72] = juce::ImageCache::getFromMemory(BinaryData::random_png, BinaryData::random_pngSize);
        prefabImages[73] = juce::ImageCache::getFromMemory(BinaryData::stereodelay_png, BinaryData::stereodelay_pngSize);
        prefabImages[74] = juce::ImageCache::getFromMemory(BinaryData::hp_png, BinaryData::hp_pngSize);
        prefabImages[75] = juce::ImageCache::getFromMemory(BinaryData::scope_png, BinaryData::scope_pngSize);
        prefabImages[76] = juce::ImageCache::getFromMemory(BinaryData::micro_png, BinaryData::micro_pngSize);
        prefabImages[77] = juce::ImageCache::getFromMemory(BinaryData::auxmodule_png, BinaryData::auxmodule_pngSize);
        prefabImages[78] = juce::ImageCache::getFromMemory(BinaryData::module_png, BinaryData::module_pngSize);
        prefabImages[79] = juce::ImageCache::getFromMemory(BinaryData::plugin_png, BinaryData::plugin_pngSize);
        prefabImages[80] = juce::ImageCache::getFromMemory(BinaryData::sequence_png, BinaryData::sequence_pngSize);
        prefabImages[81] = juce::ImageCache::getFromMemory(BinaryData::pulse_png, BinaryData::pulse_pngSize);
        prefabImages[82] = juce::ImageCache::getFromMemory(BinaryData::mixer_png, BinaryData::mixer_pngSize);
        prefabImages[83] = juce::ImageCache::getFromMemory(BinaryData::module_png, BinaryData::module_pngSize);
        prefabImages[84] = juce::ImageCache::getFromMemory(BinaryData::module_png, BinaryData::module_pngSize);
        prefabImages[85] = juce::ImageCache::getFromMemory(BinaryData::module_png, BinaryData::module_pngSize);
        prefabImages[86] = juce::ImageCache::getFromMemory(BinaryData::midiin_png, BinaryData::midiin_pngSize);
        prefabImages[87] = juce::ImageCache::getFromMemory(BinaryData::micro_png, BinaryData::micro_pngSize);
        prefabImages[88] = juce::ImageCache::getFromMemory(BinaryData::module_png, BinaryData::module_pngSize);
        prefabImages[89] = juce::ImageCache::getFromMemory(BinaryData::clock_png, BinaryData::clock_pngSize);
        prefabImages[90] = juce::ImageCache::getFromMemory(BinaryData::module_png, BinaryData::module_pngSize);
        prefabImages[91] = juce::ImageCache::getFromMemory(BinaryData::module_png, BinaryData::module_pngSize);
        prefabImages[92] = juce::ImageCache::getFromMemory(BinaryData::module_png, BinaryData::module_pngSize);
        prefabImages[93] = juce::ImageCache::getFromMemory(BinaryData::module_png, BinaryData::module_pngSize);
        prefabImages[94] = juce::ImageCache::getFromMemory(BinaryData::module_png, BinaryData::module_pngSize);
        prefabImages[95] = juce::ImageCache::getFromMemory(BinaryData::led_on_png, BinaryData::led_on_pngSize);
        prefabImages[96] = juce::ImageCache::getFromMemory(BinaryData::logic_and_png, BinaryData::logic_and_pngSize);
        prefabImages[97] = juce::ImageCache::getFromMemory(BinaryData::logic_or_png, BinaryData::logic_or_pngSize);
        prefabImages[98] = juce::ImageCache::getFromMemory(BinaryData::logic_not_png, BinaryData::logic_not_pngSize);
        prefabImages[52] = juce::ImageCache::getFromMemory(BinaryData::module_png, BinaryData::module_pngSize);
        prefabImages[53] = juce::ImageCache::getFromMemory(BinaryData::module_png, BinaryData::module_pngSize);
        
        categories = new juce::StringArray();

        categories->add("Audio");
        categories->add("Controls");
        categories->add("Filter");
        categories->add("Logic");
        categories->add("Math");
        categories->add("Input / Output");
        categories->add("MIDI");
        categories->add("Sound sources");
        categories->add("FX");
        categories->add("Debug");
        
    };
    
    ~PrefabFactory() {
        categories->clear();
        delete categories;
    };

    
    
    int numPrefabInstances = 0;
};

#endif /* PrefabFactory_hpp */
