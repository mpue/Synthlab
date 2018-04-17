//
//  Project.hpp
//  Synthlab - App
//
//  Created by Matthias Pueski on 16.04.18.
//

#ifndef Project_h
#define Project_h

#include "../JuceLibraryCode/JuceHeader.h"

class Project {
public:
    static Project* getInstance() {
        if (instance == NULL) {
            instance = new Project();
        }
        return instance;
    }
    
    static void destroy() {
        delete instance;
    }
    
    UndoManager* getUndoManager() {
        return undoManager;
    }
    
    AudioFormatManager* getFormatManager() {
        return manager;
    }
    
private:
    
    Project() {
        undoManager = new UndoManager();
        manager = new AudioFormatManager();
        manager->registerBasicFormats();
    }
    
    ~Project() {
        delete undoManager;
        delete manager;
    }
    static Project* instance;
    UndoManager* undoManager;
    
    AudioFormatManager* manager;
};

#endif /* Project_hp*/
