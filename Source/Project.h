//
//  Project.hpp
//  Synthlab - App
//
//  Created by Matthias Pueski on 16.04.18.
//

#ifndef Project_h
#define Project_h

#include "../JuceLibraryCode/JuceHeader.h"
#include "CustomLookAndFeel.h"
#include "MainTabbedComponent.h"
#include "AudioEngine/Sampler.h"
#include "PropertyView.h"
#include "Module.h"

class Project {
public:
    
    enum AppMode {
        STUDIO,
        PLAYER
    };
    
    static Project* getInstance() {
        if (instance == NULL) {
            instance = new Project();
        }
        return instance;
        
    }
    
    static void destroy() {
        delete instance;
    }

    juce::UndoManager* getUndoManager() {
        return undoManager;
    }
    
    juce::StringArray& getRecentFiles() {
        return recentFiles;
    }

    void loadRecentFileList();
    void addRecentFile(juce::String path);
    
    void setSupplemental(MainTabbedComponent* component) {
        this->supplementalTab = component;
    }
    
    MainTabbedComponent* getSupplemental(){
        return supplementalTab;
    }

    inline CustomLookAndFeel* getLookAndFeel() {
        return lookAndFeel;
    };
    
    std::vector<juce::DialogWindow*>& getOpenWindows() {
        return openWindows;
    }
    
    void setDefaultSampler(Sampler* sampler) {
        this->defaultSampler = sampler;
    }
    
    Sampler* getDefaultSampler() {
        return this->defaultSampler;
    }
    
    void setRoot(Module* root) {
        this->root = root;
    }
 
    Module* getRoot() {
        return root;
    }
    
    juce::Component* getMain() {
        return main;
    }
    
    void setMain(juce::Component* c) {
        main = c;
    }
    
    void setPropertyView(PropertyView* view) {
        this->propertyView = view;
    }
    
    PropertyView* getPropertyView() {
        return propertyView;
    }
    
    ApplicationCommandManager* getCommandManager() {
        return commandManager;
    };
    
    void setCommandManager(ApplicationCommandManager* commandManager) {
        this->commandManager = commandManager;
    }
    
    AppMode getAppMode() {
        return mode;
    }
    
#ifdef USE_PUSH
    ableton::Push2DisplayBridge*  getPush2Bridge() {
        return &bridge;
    }
#endif
    
private:
    
    Project() {
        undoManager = new juce::UndoManager();
        lookAndFeel = new CustomLookAndFeel();
    }
    
    ~Project() {
        
        // cleanup existing and opened windows
        
        for (std::vector<juce::DialogWindow*>::iterator it = openWindows.begin();it != openWindows.end();) {
            if ((*it) != nullptr) {
                if ((*it)->isVisible()){
                    (*it)->setVisible(false);
                    delete (*it);
                }
            }
            it = openWindows.erase(it);
        }
        if (supplementalTab != nullptr) {
            supplementalTab->clearTabs();
            delete supplementalTab;
        }

        delete undoManager;
        delete lookAndFeel;
    }
    
    ApplicationCommandManager* commandManager;
    
    PropertyView* propertyView = nullptr;
    static Project* instance;
    juce::UndoManager* undoManager;
    juce::StringArray recentFiles;
    CustomLookAndFeel* lookAndFeel;
    MainTabbedComponent* supplementalTab = nullptr;
    std::vector<juce::DialogWindow*> openWindows;
    juce::Component* main = nullptr;
    Sampler* defaultSampler = nullptr;
    Module* root;
    AppMode mode = PLAYER;
    
    
};

#endif /* Project_hp*/
