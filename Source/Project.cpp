//
//  Project.cpp
//  Synthlab - App
//
//  Created by Matthias Pueski on 16.04.18.
//

#include "Project.h"

Project* Project::instance = NULL;

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
    
    OutputStream* os = configFile.createOutputStream();
    
    XmlElement* xml = v->createXml();
    
    xml->writeToStream(*os, "");
    
    delete os;
    delete xml;
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
        ScopedPointer<XmlElement> xml = XmlDocument(configFile).getDocumentElement();
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
