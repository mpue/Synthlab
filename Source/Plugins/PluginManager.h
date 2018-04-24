//
//  PluginManager.hpp
//  WaveX - App
//
//  Created by Matthias Pueski on 06.04.18.
//  Copyright © 2018 pueski.de. All rights reserved.
//

#ifndef PluginManager_h
#define PluginManager_h

#include "../JuceLibraryCode/JuceHeader.h"
#include <map>

class PluginManager {
    
public:
    static PluginManager* getInstance() {
        if (instance == NULL) {
            instance = new PluginManager();
        }
        return instance;
    }
    
    static void destroy() {
        delete instance;
    }
    
    class PluginWindow    : public DocumentWindow
    {
    public:
        PluginWindow (String name, AudioProcessorEditor* editor)  : DocumentWindow (name,
                                                                                    Colours::lightgrey,
                                                                                    DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);
            this->plugin = editor;
            setContentOwned (editor, true);
            setAlwaysOnTop(true);
            setResizable (true, true);
            centreWithSize(editor->getWidth(),editor->getHeight()+25);
            //setVisible (true);
            
        }
        
        ~PluginWindow() {
            delete plugin;
        }
        
        void closeButtonPressed() override
        {
            this->setVisible(false);
        }
        
    private:
            AudioProcessorEditor* plugin;
    };
    
    void scanPlugins();
    void addPlugin(String name,AudioDeviceManager* deviceManager);
    void configureBusLayout(AudioPluginInstance* plugin ,AudioDeviceManager* deviceManager, AudioAppComponent* component);
    AudioPluginInstance* getPlugin(String name);
    PluginWindow* getPluginWindow(String name);
    PopupMenu* buildPluginMenu();
    long getNextPluginId();
    std::vector<String> getAvailablePlugins();
    StringArray& getPlugins();
    
    int getNumActiveChannels(int i) {
        i = i - ((i >> 1) & 0x55555555);
        i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
        return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
    }
    
    void updatePluginList();
    void openPluginWindow(String name, AudioDeviceManager* manager);
    
private:
    
    std::vector<String> availableInstruments;

    PluginManager();
    
    ~PluginManager(){
        for(std::map<String, AudioPluginInstance*>::iterator itr = pluginMap.begin(); itr != pluginMap.end(); itr++)
        {
      
            String name = itr->first;
            
            if (pluginMap[name] != nullptr) {
                // delete editorWindowMap[name];
                delete pluginWindowMap[name];
                pluginMap[name]->releaseResources();
                delete itr->second;
            }
            
        }
        
        if (plugins != nullptr) {
            delete plugins;
        }
        if (pluginList != nullptr) {
            pluginList->clear();
            // delete pluginList;
        }
        delete apfm;
    }
    
    std::map<String,AudioPluginInstance*> pluginMap;
    std::map<String,PluginWindow*> pluginWindowMap;
    std::map<String,AudioProcessorEditor*> editorWindowMap;
    StringArray* plugins = nullptr;
    KnownPluginList* pluginList = nullptr;
    static PluginManager* instance;
    AudioPluginFormatManager* apfm;
};

#endif /* PluginManager_h */
