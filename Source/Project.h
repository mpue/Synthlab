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
    
private:
    
    Project() {
        undoManager = new UndoManager();
    }
    
    ~Project() {
        delete undoManager;
    }
    static Project* instance;
    UndoManager* undoManager;
};

#endif /* Project_hp*/
