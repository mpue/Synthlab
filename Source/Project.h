//
//  Project.hpp
//  Synthlab - App
//
//  Created by Matthias Pueski on 16.04.18.
//

#ifndef Project_h
#define Project_h

#define USE_PUSH

#include "../JuceLibraryCode/JuceHeader.h"
#include "CustomLookAndFeel.h"
#include "MainTabbedComponent.h"
#include "AudioEngine/Sampler.h"
#include "PropertyView.h"

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
        return nullptr;// return lookAndFeel;
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
    
#ifdef USE_PUSH
    ableton::Push2DisplayBridge*  getPush2Bridge() {
        return &bridge;
    }
#endif
    
private:
    
    Project() {
        undoManager = new juce::UndoManager();
        lookAndFeel = new CustomLookAndFeel();
#ifdef USE_PUSH
        
        // First we initialise the low level push2 object
        NBase::Result result = push2Display.Init();
        
        /*
        if (result.Failed()){
            Logger::writeToLog("Failed to initialize Push2");
        }
        
        // Then we initialise the juce to push bridge
        */
        result = bridge.Init(push2Display);
        /*
        if (result.Failed()) {
             Logger::writeToLog("Failed to initialize Push display");
        }
        else {
         
            auto& g = Project::getInstance()->getPush2Bridge()->GetGraphic();
            // Clear previous frame
            g.fillAll(juce::Colour(0xff000000));
            
            // Create a path for the animated wave
            const auto height = ableton::Push2DisplayBitmap::kHeight;
            const auto width = ableton::Push2DisplayBitmap::kWidth;
            
            g.setFont(18);
             String message = "Welcome to Synthlab";
            Rectangle<int> rect = Rectangle<int>(width / 2 - g.getCurrentFont().getStringWidth(message) / 2, height / 2);
            g.drawText(message, rect, juce::Justification::centred);
        
        }
        
        
        const auto height = ableton::Push2DisplayBitmap::kHeight;
        const auto width = ableton::Push2DisplayBitmap::kWidth;
        
        Rectangle<int> pushBounds = Rectangle<int>(0,0,width,height);
        */

#endif
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
    
    PropertyView* propertyView = nullptr;
    static Project* instance;
    juce::UndoManager* undoManager;
    juce::StringArray recentFiles;
    CustomLookAndFeel* lookAndFeel;
    MainTabbedComponent* supplementalTab = nullptr;
    std::vector<juce::DialogWindow*> openWindows;
    juce::Component* main = nullptr;
    Sampler* defaultSampler = nullptr;
    
#ifdef USE_PUSH
    ableton::Push2DisplayBridge bridge;    /*!< The bridge allowing to use juce::graphics for push */
    ableton::Push2Display push2Display;
#endif
    
};

#endif /* Project_hp*/
