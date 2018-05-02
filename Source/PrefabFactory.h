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
        Prefab(String name, String category){
            this->name = name;
            this->category = category;
            this->icon = icon;
        };
        String getName() {
            return name;
        }
        String getCategory() {
            return category;
        }
        int getId() {
            return prefabId;
        }
        
        void setId(int prefabId) {
            this->prefabId = prefabId;
        }
        
        Image* getIcon() {
            return &icon;
        }
        
        void setIcon(Image& icon) {
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
        String name;
        String category;
        Image icon;
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
    
    
    Image getImage(int prefabId) {
        return prefabImages[prefabId];
    }
    
    PopupMenu createPrefabMenu();
    Module* getPrefab(int id, float sampleRate, int bufferSize);
    std::map<int,Prefab> getPrefabNames() {return prefabs;};
    StringArray* getCategories() { return categories;};
    int getNumInstances(int id);
    void reset();
private:
    static PrefabFactory* instance;
    
    std::map<int,Prefab> prefabs;
    std::map<int,Image> prefabImages;
    
    StringArray* categories;
    
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
        
        prefabImages[53] = ImageCache::getFromMemory(BinaryData::sampler_png, BinaryData::sampler_pngSize);
        prefabImages[54] = ImageCache::getFromMemory(BinaryData::module_png, BinaryData::module_pngSize);
        prefabImages[55] = ImageCache::getFromMemory(BinaryData::p2f_png, BinaryData::p2f_pngSize);
        prefabImages[56] = ImageCache::getFromMemory(BinaryData::inverter_png, BinaryData::inverter_pngSize);
        prefabImages[57] = ImageCache::getFromMemory(BinaryData::multiplier_png, BinaryData::multiplier_pngSize);
        prefabImages[58] = ImageCache::getFromMemory(BinaryData::display_png, BinaryData::display_pngSize);
        prefabImages[59] = ImageCache::getFromMemory(BinaryData::adder_png, BinaryData::adder_pngSize);
        prefabImages[60] = ImageCache::getFromMemory(BinaryData::adder_png, BinaryData::adder_pngSize);
        prefabImages[61] = ImageCache::getFromMemory(BinaryData::midiin_png, BinaryData::midiin_pngSize);
        prefabImages[62] = ImageCache::getFromMemory(BinaryData::midiout_png, BinaryData::midiout_pngSize);
        prefabImages[63] = ImageCache::getFromMemory(BinaryData::note_png, BinaryData::note_pngSize);
        prefabImages[64] = ImageCache::getFromMemory(BinaryData::saw_png, BinaryData::saw_pngSize);
        prefabImages[65] = ImageCache::getFromMemory(BinaryData::whitenoise_png, BinaryData::whitenoise_pngSize);
        prefabImages[66] = ImageCache::getFromMemory(BinaryData::speaker_png, BinaryData::speaker_pngSize);
        prefabImages[67] = ImageCache::getFromMemory(BinaryData::module_png, BinaryData::module_pngSize);
        prefabImages[68] = ImageCache::getFromMemory(BinaryData::lp_png, BinaryData::lp_pngSize);
        prefabImages[69] = ImageCache::getFromMemory(BinaryData::module_png, BinaryData::module_pngSize);
        prefabImages[70] = ImageCache::getFromMemory(BinaryData::adsr_png, BinaryData::adder_pngSize);
        prefabImages[71] = ImageCache::getFromMemory(BinaryData::sine_png, BinaryData::sine_pngSize);
        prefabImages[72] = ImageCache::getFromMemory(BinaryData::random_png, BinaryData::random_pngSize);
        prefabImages[73] = ImageCache::getFromMemory(BinaryData::stereodelay_png, BinaryData::stereodelay_pngSize);
        prefabImages[74] = ImageCache::getFromMemory(BinaryData::hp_png, BinaryData::hp_pngSize);
        prefabImages[75] = ImageCache::getFromMemory(BinaryData::scope_png, BinaryData::scope_pngSize);
        prefabImages[76] = ImageCache::getFromMemory(BinaryData::micro_png, BinaryData::micro_pngSize);
        prefabImages[77] = ImageCache::getFromMemory(BinaryData::aux_png, BinaryData::aux_pngSize);
        prefabImages[78] = ImageCache::getFromMemory(BinaryData::module_png, BinaryData::module_pngSize);
        prefabImages[79] = ImageCache::getFromMemory(BinaryData::plugin_png, BinaryData::plugin_pngSize);
        prefabImages[80] = ImageCache::getFromMemory(BinaryData::sequence_png, BinaryData::sequence_pngSize);
        prefabImages[81] = ImageCache::getFromMemory(BinaryData::pulse_png, BinaryData::pulse_pngSize);
        prefabImages[82] = ImageCache::getFromMemory(BinaryData::module_png, BinaryData::module_pngSize);
        
        categories = new StringArray();

        categories->add("Audio");
        categories->add("Controls");
        categories->add("Filter");
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
