/*
  ==============================================================================

    Project.cpp
    Created: 21 Dec 2016 8:18:45pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "Project.h"

using juce::String;
using juce::File;
using juce::ValueTree;
using juce::ScopedPointer;
using juce::XmlElement;
using juce::XmlDocument;
using juce::OutputStream;


Project *Project::instance = NULL;
long  Project::DEFAULT_TRACK_LENGTH = 600;
int   Project::DEFAULT_TRACK_HEIGHT = 280;
float Project::DEFAULT_TEMPO = 120.0f;

void Project::addRecentFile(juce::String path) {
    
    if (recentFiles.size() > 5) {
        recentFiles.remove(0);
    }
    
    if (!recentFiles.contains(path)) {
        recentFiles.add(path);
    }
    
    String userHome = File::getSpecialLocation(File::userHomeDirectory).getFullPathName();
    File appDir = File(userHome+"/.Synthlab");
    
    if (!appDir.exists()) {
        appDir.createDirectory();
    }
    
    File configFile = File(userHome+"/.Synthlab/recent.xml");
    
    if (!configFile.exists()) {
        configFile.create();
    }
    else {
        configFile.deleteFile();
        configFile = File(userHome+"/.Synthlab/recent.xml");
    }
    
    ValueTree* v = new ValueTree("RecentFiles");
    
    for (int i = 0 ;i < recentFiles.size();i++) {
        
        ValueTree child = ValueTree("File");
        child.setProperty("path", recentFiles.getReference(i), nullptr);
        v->addChild(child, i, nullptr);
    }
    
    std::unique_ptr<OutputStream> os = configFile.createOutputStream();
    
    std::unique_ptr<XmlElement> xml = v->createXml();
    
    xml->writeToStream(*os, "");

    os = nullptr;
    xml = nullptr;
    delete v;
    
}



void Project::loadRecentFileList() {
    
    String userHome = File::getSpecialLocation(File::userHomeDirectory).getFullPathName();
    
    File appDir = File(userHome+"/.Synthlab");
    
    if (!appDir.exists()) {
        appDir.createDirectory();
    }
    
    File configFile = File(userHome+"/.Synthlab/recent.xml");
    
    if (configFile.exists()) {
        std::unique_ptr<XmlElement> xml = XmlDocument(configFile).getDocumentElement();
        ValueTree v = ValueTree::fromXml(*xml.get());
        
        for (int i = 0 ;i < v.getNumChildren();i++) {
            String path = v.getChild(i).getProperty("path");
            
            if(!recentFiles.contains(path)) {
                recentFiles.add(path);
            }
        }
        xml = nullptr;
    }
    
    
}

void Project::destroy() {
    delete instance;
}

juce::UndoManager* Project::getUndoManager() {
    return undoManager;
}

juce::StringArray& Project::getRecentFiles() {
    return recentFiles;
}


void Project::setSupplemental(MainTabbedComponent* component) {
    this->supplementalTab = component;
}

MainTabbedComponent* Project::getSupplemental(){
    return supplementalTab;
}

CustomLookAndFeel* Project::getLookAndFeel() {
    return lookAndFeel;
};

std::vector<juce::DialogWindow*>& Project::getOpenWindows() {
    return openWindows;
}

void Project::setDefaultSampler(Sampler* sampler) {
    this->defaultSampler = sampler;
}

Sampler* Project::getDefaultSampler() {
    return this->defaultSampler;
}

void Project::setRoot(Module* root) {
    this->root = root;
}

Module* Project::getRoot() {
    return root;
}

juce::Component* Project::getMain() {
    return main;
}

void Project::setMain(juce::Component* c) {
    main = c;
}

void Project::setPropertyView(PropertyView* view) {
    this->propertyView = view;
}

PropertyView* Project::getPropertyView() {
    return propertyView;
}

ApplicationCommandManager* Project::getCommandManager() {
    return commandManager;
};

void Project::setCommandManager(ApplicationCommandManager* commandManager) {
    this->commandManager = commandManager;
}

Project::AppMode Project::getAppMode() {
    return mode;
}

void Project::setNewFile(bool isNew) {
    this->newFile = isNew;
}

bool Project::isNewFile() {
    return newFile;
}

void Project::setCurrentFilePath(juce::String path) {
    this->currentFilePath = path;
}

String Project::getCurrentFilePath() {
    return currentFilePath;
}

void Project::setCurrentFileName(juce::String path) {
    this->currentFileName = path;
}

String Project::getCurrentFileName() {
    return currentFileName;
}

bool Project::isDirty() {
    return dirty;
}

void Project::setDirty(bool dirty) {
    this->dirty = dirty;
}
