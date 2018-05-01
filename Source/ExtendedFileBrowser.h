//
//  ExtendedFileBrowser.hpp
//  Synthlab - App
//
//  Created by Matthias Pueski on 30.04.18.
//  Copyright © 2018 Pueski. All rights reserved.
//

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "AudioEngine/Sampler.h"

class FileBrowserModel : public TableListBoxModel {
public:
    
    FileBrowserModel(DirectoryContentsList* directoryList, File& initalDir);
    
    virtual void paintRowBackground (Graphics& g,
                                     int rowNumber,
                                     int width, int height,
                                     bool rowIsSelected) override;
    virtual void paintCell (Graphics& g,
                            int rowNumber,
                            int columnId,
                            int width, int height,
                            bool rowIsSelected) override;
    virtual int getNumRows() override;
    void setCurrentDir(File* dir);
    void update();
    DirectoryContentsList* getDirectoryList();
    
    String& getCurrentDir() {
        return currentDirectory;
    }
    
    var getDragSourceDescription (const SparseSet<int>& currentlySelectedRows) override{
        
        if (!currentlySelectedRows.getTotalRange().isEmpty()) {
            File file = directoryList->getFile(currentlySelectedRows.getTotalRange().getStart());
            return file.getFullPathName();
        }
        
        return var();
    }
    
private:
    DirectoryContentsList* directoryList;
    String currentDirectory;

};

class ExtendedFileBrowser : public Component, public DragAndDropContainer, public ChangeListener, public Timer{
    
    
public:
    ExtendedFileBrowser(const File& initialFileOrDirectory,const WildcardFileFilter* fileFilter, FileBrowserModel* model);
    ~ExtendedFileBrowser();
    
    void mouseDrag (const MouseEvent& event) override;
    void mouseDoubleClick(const MouseEvent& event) override;
    void mouseDown(const MouseEvent& event) override;
    void paint (Graphics& g) override;
    void resized() override;
    virtual void changeListenerCallback (ChangeBroadcaster* source) override;
    TableListBox* getTable() {
        return table;
    };
    virtual void timerCallback() override;
    
private:
    FileBrowserModel* model = nullptr;
    const File& initialDir;
    const WildcardFileFilter* filter;
    TableListBox* table = nullptr;
    Viewport* view = nullptr;
    Sampler* sampler;
    bool playing = false;
};

