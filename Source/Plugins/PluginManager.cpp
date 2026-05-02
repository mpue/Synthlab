//
//  PluginManager.cpp
//  WaveX - App
//
//  Created by Matthias Pueski on 06.04.18.
//  Copyright © 2018 pueski.de. All rights reserved.
//

#include "PluginManager.h"
#include "PluginManagerDialog.h"
#include "../Project/Project.h"

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
    addDefaultFormatsToManager(*apfm);
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
    auto props = std::make_unique<PropertiesFile>(file, options);

    auto* dialog = PluginManagerDialog::show(*apfm, *pluginList, props.get(),
                                             Project::getInstance()->getLookAndFeel());

    // Persist scan results when the user closes the dialog. We can't block
    // here (the dialog is async), so attach a callback that fires on close.
    auto saveResults = [list = pluginList, userHome]()
    {
        File pluginDir = File(userHome + "/.Synthlab/plugins");
        if (!pluginDir.exists())
            pluginDir.createDirectory();

        // Wipe stale XML files so removed plugins don't keep getting picked up
        // by updatePluginList() on next launch.
        juce::Array<File> existing;
        pluginDir.findChildFiles(existing, File::findFiles, false);
        for (auto& f : existing)
            f.deleteFile();

        for (int i = 0; i < list->getNumTypes(); ++i)
        {
            auto xml = list->getType(i)->createXml();
            xml->writeToFile(File(userHome + "/.Synthlab/plugins/" + list->getType(i)->name), "");
        }
    };

    if (dialog != nullptr)
    {
        juce::ModalComponentManager::getInstance()->attachCallback(
            dialog,
            juce::ModalCallbackFunction::create([saveResults, propsRaw = props.release()](int)
            {
                saveResults();
                propsRaw->saveIfNeeded();
                delete propsRaw;
            }));
    }
    else
    {
        // Fallback: dialog couldn't be created — keep old behaviour so the
        // user still has *some* feedback.
        saveResults();
    }
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
    
    pluginList = std::shared_ptr<KnownPluginList>(new KnownPluginList());
    
    ScopedPointer<DirectoryIterator> iter = new DirectoryIterator(presetPath, false);
    
    while(iter->next()) {
        std::unique_ptr<XmlElement> xml = XmlDocument(iter->getFile()).getDocumentElement();
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
    std::unique_ptr<XmlElement> xml = XmlDocument(preset).getDocumentElement();
    pd.loadFromXml(*xml.get());
    
    std::unique_ptr<AudioPluginInstance> plugin = apfm->createPluginInstance(pd, deviceManager->getCurrentAudioDevice()->getCurrentSampleRate(),deviceManager->getCurrentAudioDevice()->getCurrentBufferSizeSamples(),error);
    plugin->prepareToPlay(deviceManager->getCurrentAudioDevice()->getCurrentSampleRate(),deviceManager->getCurrentAudioDevice()->getCurrentBufferSizeSamples());
    pluginMap.insert(std::make_pair(name, plugin.get()));

    // configureBusLayout(plugin, deviceManager,component);
    if (pluginMap.find(name) != pluginMap.end()) {
        // do we have an editor window?

    }
    
}

PluginManager::PluginWindow* PluginManager::getPluginWindow(juce::String name) {
    // no matching plugin
    return nullptr;
}

AudioPluginInstance* PluginManager::getPlugin(juce::String name) {
    return pluginMap[name];
}

long PluginManager::getNextPluginId() {
    
    // first 10 ids are reserved for default channels
    return (long)pluginMap.size() + 10;
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
            std::unique_ptr<XmlElement> xml = XmlDocument(iter->getFile()).getDocumentElement();
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
            std::unique_ptr<XmlElement> xml = XmlDocument(iter->getFile()).getDocumentElement();
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

void PluginManager::mouseDown(const MouseEvent& event)
{
    if (pluginListComponent != nullptr) {

    }
}
