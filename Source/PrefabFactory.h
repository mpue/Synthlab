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
        };
        String getName() {
            return name;
        }
        String getCategory() {
            return category;
        }
        
        int getNumInstances() {
            return numInstances;
        }
        
        void addInstance() {
            numInstances++;
        }
        
    private:
        int numInstances = 0;
        String name;
        String category;
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
    
    PopupMenu createPrefabMenu();
    Module* getPrefab(int id, float sampleRate, int bufferSize);
    std::map<int,Prefab> getPrefabNames() {return prefabs;};
    StringArray* getCategories() { return categories;};
    
private:
    static PrefabFactory* instance;
    
    std::map<int,Prefab> prefabs;
    StringArray* categories;
    
    PrefabFactory() {

        prefabs[60] = Prefab("Adder","Math");
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
        
        categories = new StringArray();

        categories->add("Controls");
        categories->add("Filter");
        categories->add("Math");
        categories->add("Input / Output");
        categories->add("MIDI");
        categories->add("Sound sources");
        categories->add("FX");
        
    };
    ~PrefabFactory() {
        categories->clear();
        delete categories;
    };
    
    int numPrefabInstances = 0;
};

#endif /* PrefabFactory_hpp */
