//
//  PluginManager.cpp
//  WaveX - App
//
//  Created by Matthias Pueski on 06.04.18.
//  Copyright © 2018 pueski.de. All rights reserved.
//

#include "PluginManager.h"

PluginManager* PluginManager::instance = NULL;

PluginManager::PluginManager() {
    apfm = new AudioPluginFormatManager();
    apfm->addDefaultFormats();
}

void PluginManager::scanPlugins() {
    
    AudioPluginFormatManager* apfm = new AudioPluginFormatManager();
    apfm->addDefaultFormats();
    
    KnownPluginList *pluginList = new KnownPluginList();
    PropertiesFile::Options options;
    
    String userHome = File::getSpecialLocation(File::userHomeDirectory).getFullPathName();
    
    File appDir = File(userHome+"/.Synthlab");
    
    if (!appDir.exists()) {
        appDir.createDirectory();
    }
    
    File file = File(userHome+"/.Synthlab/plugin.properties");
    PropertiesFile* props = new PropertiesFile(file,options);
    
    PluginListComponent* pluginListComponent = new PluginListComponent(*apfm,*pluginList,File(), props);
    
    DialogWindow::LaunchOptions launchOptions;
    launchOptions.dialogTitle = ("Manage Plugins");
    launchOptions.escapeKeyTriggersCloseButton = true;
    launchOptions.resizable = true;
    launchOptions.useNativeTitleBar = false;
    launchOptions.useBottomRightCornerResizer = true;
    launchOptions.componentToCentreAround = nullptr;
    launchOptions.content.setOwned(pluginListComponent);
    launchOptions.content->setSize(600, 580);
    launchOptions.runModal();
    
    File pluginDir = File(userHome+"/.Synthlab/plugins");
    
    if (!pluginDir.exists()) {
        pluginDir.createDirectory();
    }
    
    for (int i = 0; i < pluginList->getNumTypes();i++) {
        pluginList->getType(i)->createXml()->writeToFile(File(userHome+"/.Synthlab/plugins/"+pluginList->getType(i)->name),"");
    }

}


void PluginManager::configureBusLayout(AudioPluginInstance* plugin, AudioDeviceManager* deviceManager, AudioAppComponent* component) {
    juce::AudioProcessor::BusesLayout layout =  plugin->getBusesLayout();
    
    int numPluginInputs = plugin->getBusCount(true) * 2;
    int numPluginOutputs = plugin->getBusCount(false) * 2;
    
    // check input and output bus configuration
    
    
    juce::BigInteger activeInputChannels = deviceManager->getCurrentAudioDevice()->getActiveInputChannels();
    juce::BigInteger activeOutputChannels = deviceManager->getCurrentAudioDevice()->getActiveOutputChannels();
    
    int numInputChannels = deviceManager->getCurrentAudioDevice()->getInputChannelNames().size();
    int numOutputChannels = deviceManager->getCurrentAudioDevice()->getOutputChannelNames().size();
    
    
    int numActiveHostInputs = getNumActiveChannels(activeInputChannels.toInteger());
    int numActiveHostOutputs = getNumActiveChannels(activeOutputChannels.toInteger());
    
    int requestedInputChannelSize = numActiveHostInputs;
    int requestedOutputChannelSize = numActiveHostOutputs;
    
    // we have a problem if a plugin has more inputs and outputs that are active
    // try to set the num of inputs/ouputs accordingly, if this fails we are fucked anyway
    
    if (numPluginInputs > numActiveHostInputs) {
        requestedInputChannelSize = numPluginInputs;
    }
    if (numPluginOutputs > numActiveHostOutputs) {
        requestedOutputChannelSize = numPluginOutputs;
    }
    
    component->setAudioChannels(requestedInputChannelSize, requestedOutputChannelSize);
    
    Logger::getCurrentLogger()->writeToLog("Active output channels "+String(numActiveHostOutputs));
    Logger::getCurrentLogger()->writeToLog("Active input channels "+String(numActiveHostInputs));
}


void PluginManager::addPlugin(String name, AudioDeviceManager* deviceManager) {
    

    String error = String("Error");
    PluginDescription pd;
    
    String userHome = File::getSpecialLocation(File::userHomeDirectory).getFullPathName();
    
    File preset = File(userHome+"/.Synthlab/plugins/"+name);
    ScopedPointer<XmlElement> xml = XmlDocument(preset).getDocumentElement();
    pd.loadFromXml(*xml.get());
    
    AudioPluginInstance* plugin = apfm->createPluginInstance(pd, deviceManager->getCurrentAudioDevice()->getCurrentSampleRate(),deviceManager->getCurrentAudioDevice()->getCurrentBufferSizeSamples(),error);
    plugin->prepareToPlay(deviceManager->getCurrentAudioDevice()->getCurrentSampleRate(),deviceManager->getCurrentAudioDevice()->getCurrentBufferSizeSamples());
    AudioProcessorEditor* editor = plugin->createEditorIfNeeded();
    
    // configureBusLayout(plugin, deviceManager,component);
    
    pluginMap[name] = plugin;
    
    PluginWindow* win = new PluginWindow(name,editor);
    pluginWindowMap[name] = win;
    
}

PluginManager::PluginWindow* PluginManager::getPluginWindow(String name) {
    return pluginWindowMap[name];
}

AudioPluginInstance* PluginManager::getPlugin(juce::String name) {
    return pluginMap[name];
}

long PluginManager::getNextPluginId() {
    
    // first 10 ids are reserved for default channels
    return pluginMap.size() + 10;
}

void PluginManager::cleanup() {
    
    for(std::map<String, PluginWindow*>::iterator itr = pluginWindowMap.begin(); itr != pluginWindowMap.end(); itr++)
    {
        if (itr->second->isVisible()) {
            itr->second->setVisible(false);
        }
        delete (itr->second);
    }
    
}

PopupMenu* PluginManager::buildPluginMenu() {
    
    PopupMenu* menu = new PopupMenu();
    
    String appDataPath = File::getSpecialLocation(File::userApplicationDataDirectory).getFullPathName();
    String userHome = File::getSpecialLocation(File::userHomeDirectory).getFullPathName();
    
    String presetPath = userHome+"/.Synthlab/plugins";
    File presets = File(presetPath);
    
    if (!presets.exists()) {
        presets.createDirectory();
    }
    
    int index = 100;
    
    StringArray manufacturers = StringArray();
    
    if(presets.exists() && presets.isDirectory()) {
        
        ScopedPointer<DirectoryIterator> iter = new DirectoryIterator(presets, false);
        
        while(iter->next()) {
            ScopedPointer<XmlElement> xml = XmlDocument(iter->getFile()).getDocumentElement();
            PluginDescription pd;
            pd.loadFromXml(*xml.get());
            
            if (pd.isInstrument) {
                if (!manufacturers.contains(pd.manufacturerName)) {
                    manufacturers.add(pd.manufacturerName);
                }
            }
            
        }
        iter = nullptr;
        
    }
    
    std::map<std::string,PopupMenu> menuMap;
    
    for (int i = 0; i < manufacturers.size();i++) {
        PopupMenu sub = PopupMenu();
        menuMap[manufacturers.getReference(i).toStdString()] = sub;
    }
    
    if(presets.exists() && presets.isDirectory()) {
        
        ScopedPointer<DirectoryIterator> iter = new DirectoryIterator(presets, false);
        
        while(iter->next()) {
            ScopedPointer<XmlElement> xml = XmlDocument(iter->getFile()).getDocumentElement();
            PluginDescription pd;
            pd.loadFromXml(*xml.get());
            
            if (!pd.isInstrument) {
                availableInstruments.push_back(pd.name);
                menuMap[pd.manufacturerName.toStdString()].addItem(index, pd.name, true, false,nullptr);
                index++;
            }
            
        }
        iter = nullptr;
        
        for (int i = 0; i < manufacturers.size();i++) {
            menu->addSubMenu(manufacturers.getReference(i),menuMap[manufacturers.getReference(i).toStdString()]);
        }
        
    }
    
    return menu;
}

std::vector<String> PluginManager::getAvailablePlugins() {
    return availableInstruments;
}

void PluginManager::releasePlugin(String name) {
    delete pluginWindowMap[name];
    if (pluginMap[name] != nullptr) {
        
        if (pluginMap[name]->getActiveEditor() != nullptr) {
            delete pluginMap[name]->getActiveEditor();
        }

        pluginMap[name]->releaseResources();
        delete pluginMap[name] ;
        
    }
}

void PluginManager::openPluginWindow(juce::String name, AudioDeviceManager* manager) {
    // addPlugin(name,manager);
    // AudioPluginInstance* plugin = getPlugin(name);
    // getPluginWindow(name)->setVisible(true);
}
