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
    
    static void destroy();

    juce::UndoManager* getUndoManager();
    juce::StringArray& getRecentFiles();
    void loadRecentFileList();
    void addRecentFile(juce::String path);

    void setSupplemental(MainTabbedComponent* component) ;
    MainTabbedComponent* getSupplemental();

    CustomLookAndFeel* getLookAndFeel();
    
    std::vector<juce::DialogWindow*>& getOpenWindows();

    void setDefaultSampler(Sampler* sampler);
    Sampler* getDefaultSampler();
    
    void setRoot(Module* root);
    Module* getRoot();
    
    juce::Component* getMain();
    void setMain(juce::Component* c);
    
    void setPropertyView(PropertyView* view);
    PropertyView* getPropertyView();
    
    ApplicationCommandManager* getCommandManager();
    void setCommandManager(ApplicationCommandManager* commandManager);
    
    AppMode getAppMode();

    String getCurrentFilePath();
    void setCurrentFilePath(String path);
    
    String getCurrentFileName();
    void setCurrentFileName(String path);
    
    bool isNewFile();
    void setNewFile(bool isNew);
    
    bool isDirty();
    void setDirty(bool dirty);
    
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
    AppMode mode = STUDIO;
    
    String currentFilePath;
    String currentFileName;
    bool newFile = true;
    bool dirty = false;
};

#endif /* Project_hp*/
