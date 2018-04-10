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
    std::map<int,String> getPrefabNames() {return prefabs;};
    
private:
    static PrefabFactory* instance;
    
    std::map<int,String> prefabs;
    
    PrefabFactory() {
        prefabs[60] = "AdderModule";
        prefabs[61] = "MidiGate";
        prefabs[62] = "MidiOut";
        prefabs[63] = "MidiNote";
        prefabs[64] = "SawtoothModule";
        prefabs[65] = "NoiseModule";
        prefabs[66] = "AudioOut";
        prefabs[67] = "Constant";

    };
    ~PrefabFactory();
    
    int numPrefabInstances = 0;
};

#endif /* PrefabFactory_hpp */
