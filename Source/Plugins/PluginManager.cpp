//
//  PluginManager.cpp
//  WaveX - App
//
//  Created by Matthias Pueski on 06.04.18.
//  Copyright © 2018 pueski.de. All rights reserved.
//

#include "PluginManager.h"
#include "../Project.h"

using juce::AudioPluginFormatManager;
using juce::PropertiesFile;
using juce::File;
using juce::String;
using juce::PluginListComponent;
using juce::PluginDescription;
using juce::DialogWindow;
using juce::ScopedPointer;
using juce::XmlElement;
using juce::XmlDocument;
using juce::Logger;
using juce::AudioPluginInstance;
using juce::AudioDeviceManager;
using juce::AudioAppComponent;
using juce::KnownPluginList;
using juce::DirectoryIterator;
using juce::StringArray;
using juce::PopupMenu;
using juce::AudioProcessorEditor;

PluginManager* PluginManager::instance = NULL;

PluginManager::PluginManager() {
    apfm = new AudioPluginFormatManager();
    apfm->addDefaultFormats();
}

void PluginManager::scanPlugins() {
    
    if (pluginList == nullptr) {
        updatePluginList();
    }

    PropertiesFile::Options options;
    
    String userHome = File::getSpecialLocation(File::userHomeDirectory).getFullPathName();
    
    File appDir = File(userHome+"/.Synthlab");
    
    if (!appDir.exists()) {
        appDir.createDirectory();
    }
    
    File file = File(userHome+"/.Synthlab/plugin.properties");
    PropertiesFile* props = new PropertiesFile(file,options);
    
    PluginListComponent* pluginListComponent = new PluginListComponent(*apfm,*pluginList,File(), props);
    pluginListComponent->setLookAndFeel(Project::getInstance()->getLookAndFeel());
    
    DialogWindow::LaunchOptions launchOptions;
    launchOptions.dialogTitle = ("Manage Plugins");
    launchOptions.escapeKeyTriggersCloseButton = true;
    launchOptions.resizable = true;
    launchOptions.useNativeTitleBar = false;
    launchOptions.useBottomRightCornerResizer = true;
    launchOptions.componentToCentreAround = nullptr;
    launchOptions.content.setOwned(pluginListComponent);
    launchOptions.content->setSize(600, 580);
    Project::getInstance()->getOpenWindows().push_back(launchOptions.launchAsync());
    
    File pluginDir = File(userHome+"/.Synthlab/plugins");
    
    if (!pluginDir.exists()) {
        pluginDir.createDirectory();
    }
    
    for (int i = 0; i < pluginList->getNumTypes();i++) {
        ScopedPointer<XmlElement> xml = pluginList->getType(i)->createXml();
        
        xml->writeToFile(File(userHome+"/.Synthlab/plugins/"+pluginList->getType(i)->name),"");
        xml = nullptr;
    }
    
    delete pluginListComponent;
    delete props;

}


void PluginManager::configureBusLayout(AudioPluginInstance* plugin, AudioDeviceManager* deviceManager, AudioAppComponent* component) {
    juce::AudioProcessor::BusesLayout layout =  plugin->getBusesLayout();
    
    int numPluginInputs = plugin->getBusCount(true) * 2;
    int numPluginOutputs = plugin->getBusCount(false) * 2;
    
    // check input and output bus configuration
    
    
    juce::BigInteger activeInputChannels = deviceManager->getCurrentAudioDevice()->getActiveInputChannels();
    juce::BigInteger activeOutputChannels = deviceManager->getCurrentAudioDevice()->getActiveOutputChannels();
    
    /*
    int numInputChannels = deviceManager->getCurrentAudioDevice()->getInputChannelNames().size();
    int numOutputChannels = deviceManager->getCurrentAudioDevice()->getOutputChannelNames().size();
    */
    
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
    
    Logger::getCurrentLogger()->writeToLog("Active output channels "+juce::String(numActiveHostOutputs));
    Logger::getCurrentLogger()->writeToLog("Active input channels "+juce::String(numActiveHostInputs));
}

void PluginManager::updatePluginList() {
    
    juce::String userHome = File::getSpecialLocation(File::userHomeDirectory).getFullPathName();
    File presetPath = File(userHome+"/.Synthlab/plugins/");
    
    pluginList = new KnownPluginList();
    
    ScopedPointer<DirectoryIterator> iter = new DirectoryIterator(presetPath, false);
    
    while(iter->next()) {
        ScopedPointer<XmlElement> xml = XmlDocument(iter->getFile()).getDocumentElement();
        PluginDescription pd = PluginDescription();
        pd.loadFromXml(*xml);
        pluginList->addType(pd);
        xml = nullptr;
    }
    iter = nullptr;
}

void PluginManager::addPlugin(juce::String name, AudioDeviceManager* deviceManager) {
    
    juce::String error = juce::String("Error");
    PluginDescription pd;
    
    juce::String userHome = File::getSpecialLocation(File::userHomeDirectory).getFullPathName();
    
    File preset = File(userHome+"/.Synthlab/plugins/"+name);
    ScopedPointer<XmlElement> xml = XmlDocument(preset).getDocumentElement();
    pd.loadFromXml(*xml.get());
    
    AudioPluginInstance* plugin = apfm->createPluginInstance(pd, deviceManager->getCurrentAudioDevice()->getCurrentSampleRate(),deviceManager->getCurrentAudioDevice()->getCurrentBufferSizeSamples(),error);
    plugin->prepareToPlay(deviceManager->getCurrentAudioDevice()->getCurrentSampleRate(),deviceManager->getCurrentAudioDevice()->getCurrentBufferSizeSamples());
    AudioProcessorEditor* editor = plugin->createEditorIfNeeded();
    
    // configureBusLayout(plugin, deviceManager,component);
    
    
    if ( pluginMap.find(name) == pluginMap.end() ) {
        editorWindowMap[name] = editor;
        pluginMap[name] = plugin;

    }
    if (pluginWindowMap.find(name) == pluginWindowMap.end()) {
        PluginWindow* win = new PluginWindow(name,editor);
        pluginWindowMap[name] = win;
    }

    
}

PluginManager::PluginWindow* PluginManager::getPluginWindow(juce::String name) {
    return pluginWindowMap[name];
}

AudioPluginInstance* PluginManager::getPlugin(juce::String name) {
    return pluginMap[name];
}

long PluginManager::getNextPluginId() {
    
    // first 10 ids are reserved for default channels
    return pluginMap.size() + 10;
}



PopupMenu* PluginManager::buildPluginMenu() {
    
    PopupMenu *menu = new PopupMenu();
    
    juce::String appDataPath = File::getSpecialLocation(File::userApplicationDataDirectory).getFullPathName();
    juce::String userHome = File::getSpecialLocation(File::userHomeDirectory).getFullPathName();
    
    juce::String presetPath = userHome+"/.Synthlab/plugins";
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
            PluginDescription pd = PluginDescription();
            pd.loadFromXml(*xml);
            

            if (!manufacturers.contains(pd.manufacturerName)) {
                manufacturers.add(pd.manufacturerName);
            }
            xml = nullptr;
            
        }
        iter = nullptr;
        
    }
    
    std::map<juce::String,PopupMenu> menuMap;
    
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
            

            availableInstruments.push_back(pd.name);
            menuMap[pd.manufacturerName.toStdString()].addItem(index, pd.name, true, false,nullptr);
            index++;
            
            
        }
        iter = nullptr;
        
        for (int i = 0; i < manufacturers.size();i++) {
            menu->addSubMenu(manufacturers.getReference(i),menuMap[manufacturers.getReference(i).toStdString()]);
        }
        
    }
    
    return menu;
}

std::vector<juce::String> PluginManager::getAvailablePlugins() {
    return availableInstruments;
}

StringArray& PluginManager::getPlugins() {
    
    if (plugins == nullptr) {
        plugins = new StringArray();
    
        for (int i = 0; i < availableInstruments.size();i++) {
            plugins->add(availableInstruments.at(i));
        }
        
        plugins->sort(true);
    }
    return* plugins;
}


void PluginManager::openPluginWindow(juce::String name, AudioDeviceManager* manager) {
    // addPlugin(name,manager);
    // AudioPluginInstance* plugin = getPlugin(name);
    // getPluginWindow(name)->setVisible(true);
}
