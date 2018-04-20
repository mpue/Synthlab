//
//  Project.hpp
//  Synthlab - App
//
//  Created by Matthias Pueski on 16.04.18.
//

#ifndef Project_h
#define Project_h

// #define USE_PUSH

#include "../JuceLibraryCode/JuceHeader.h"

#ifdef USE_PUSH
#include "push2/JuceToPush2DisplayBridge.h"
#endif

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
    
    AudioDeviceManager* getDeviceManager() {
        return deviceManager;
    }
    
    void setDeviceManager(AudioDeviceManager* manager) {
        this->deviceManager = manager;
    }
    
    StringArray& getRecentFiles() {
        return recentFiles;
    }

    void loadRecentFileList();
    void addRecentFile(String path);

    
#ifdef USE_PUSH
    ableton::Push2DisplayBridge*  getPush2Bridge() {
        return &bridge;
    }
#endif
    
private:
    
    Project() {
        undoManager = new UndoManager();
        manager = new AudioFormatManager();
        manager->registerBasicFormats();
#ifdef USE_PUSH
        
        // First we initialise the low level push2 object
        // First we initialise the low level push2 object
        NBase::Result result = push2Display.Init();
        // RETURN_IF_FAILED_MESSAGE(result, "Failed to init push2");
        
        // Then we initialise the juce to push bridge
        result = bridge.Init(push2Display);
        // RETURN_IF_FAILED_MESSAGE(result, "Failed to init bridge");
        
        const auto height = ableton::Push2DisplayBitmap::kHeight;
        const auto width = ableton::Push2DisplayBitmap::kWidth;
        
        Rectangle<int> pushBounds = Rectangle<int>(0,0,width,height);
#endif
    }
    
    ~Project() {

        delete undoManager;
        delete manager;
    }
    
    static Project* instance;
    UndoManager* undoManager;
    AudioDeviceManager* deviceManager;
    StringArray recentFiles;
    
#ifdef USE_PUSH
    ableton::Push2DisplayBridge bridge;    /*!< The bridge allowing to use juce::graphics for push */
    ableton::Push2Display push2Display;
#endif
    AudioFormatManager* manager;
};

#endif /* Project_hp*/
