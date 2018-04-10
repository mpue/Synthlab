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
    
    Module* getPrefab(String name, float sampleRate, int bufferSize);
    
private:
    static PrefabFactory* instance;
    

    
    PrefabFactory();
    ~PrefabFactory();
};

#endif /* PrefabFactory_hpp */
