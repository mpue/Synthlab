//
//  ExtendedFileBrowser.cpp
//  Synthlab - App
//
//  Created by Matthias Pueski on 30.04.18.
//  Copyright © 2018 Pueski. All rights reserved.
//

#include "ExtendedFileBrowser.h"
#include "../JuceLibraryCode/JuceHeader.h"
#include "Project.h"
#include "AudioManager.h"

ExtendedFileBrowser::ExtendedFileBrowser(const File& initialFileOrDirectory,const WildcardFileFilter* fileFilter, FileBrowserModel* model) : initialDir(initialFileOrDirectory) {
    
    // addMouseListener(this, true);
    table = new TableListBox();
    table->getHeader().addColumn("File", 1, 350);
    table->getHeader().addColumn("Size", 2, 50);
    table->setModel(model);
    this->model = model;
    view  = new Viewport();
    view->setViewedComponent(table);
    addAndMakeVisible(view);
    
    sampler = Project::getInstance()->getDefaultSampler();
    
    model->update();

    repaint();
}

ExtendedFileBrowser::~ExtendedFileBrowser() {
    delete table;
    delete view;
}

void ExtendedFileBrowser::mouseDrag(const juce::MouseEvent &event) {
    /*
    var description;
    
    // setDragImageForIndex(0, ImageCache::getFromMemory(BinaryData::module_png, BinaryData::module_pngSize));
    startDragging(description,table);
     */

}

void ExtendedFileBrowser::paint(juce::Graphics &g) {
    g.fillAll (Colour (0xff323e44));
    g.fillRect (getLocalBounds());
    Component::paint(g);
}

void ExtendedFileBrowser::resized() {
    if (getParentComponent() != nullptr) {
        setSize(getParentWidth(), getParentHeight());
        view->setSize(getWidth(), getHeight());
        table->setSize(getWidth(), getHeight());
    }
}

void ExtendedFileBrowser::changeListenerCallback (ChangeBroadcaster* source) {
    table->updateContent();
}

void ExtendedFileBrowser::mouseDown(const juce::MouseEvent &event) {

}

void ExtendedFileBrowser::mouseDoubleClick(const juce::MouseEvent &event) {
    
    if (table->getSelectedRow() > 0) {
        File* f = new File(model->getDirectoryList()->getFile(table->getSelectedRow()));
        if (f->exists()) {
            if (f->isDirectory()) {
                model->setCurrentDir(f);
            }
            else {
                if (f->getFileExtension().toLowerCase().contains("wav")) {
                     Project::getInstance()->getDefaultSampler()->loadSample(*f);
                     Project::getInstance()->getDefaultSampler()->play();
                }
            }
                
        }
        delete f;
    }
    else {
        File* current = new File(model->getCurrentDir());
        File* parent = new File(current->getParentDirectory());
        model->setCurrentDir(parent);
        delete current;
    }

}

void ExtendedFileBrowser::timerCallback() {
   
}

//===========================================================================
// Model
//===========================================================================

FileBrowserModel::FileBrowserModel(DirectoryContentsList* directoryList, File& initalDir) {
    this->directoryList = directoryList;
    this->currentDirectory = initalDir.getFullPathName();
    directoryList->setDirectory(initalDir, true,true);
}

int FileBrowserModel::getNumRows() {
    return directoryList->getNumFiles() + 1;
}
void FileBrowserModel::paintCell (Graphics& g,
                int rowNumber,
                int columnId,
                int width, int height,
                bool rowIsSelected) {
    
    g.setColour(juce::Colours::black);
    
    String text = "";
    
    if (columnId == 1) {
 
        if (rowNumber > 0) {
            text = directoryList->getFile(rowNumber).getFileName();
        }
        else {
            text = "[..]";
        }
        g.setColour(juce::Colours::white);
        g.drawText(text, 0,0, width,height, juce::Justification::centredLeft);
       
    }
    else if (columnId == 2) {
        if (rowNumber > 0) {
            text = String(directoryList->getFile(rowNumber).getSize() / 1024) + "kB";
        }
        else {
            text = "";
        }
        
        g.setColour(juce::Colours::white);
        g.drawText(text, 0,0, width,height, juce::Justification::right);
    }

    
    
}

void FileBrowserModel::paintRowBackground (Graphics& g,
                         int rowNumber,
                         int width, int height,
                         bool rowIsSelected) {

    
    if (rowIsSelected) {
        g.setColour(juce::Colours::orange);
    }
    else {
        g.setColour(Colour (0xff2e373b));
    }
    
    g.fillRect(0,0,width,height);
}

void FileBrowserModel::setCurrentDir(juce::File* dir){
    this->currentDirectory = dir->getFullPathName();
    directoryList->setDirectory(*dir, true, true);
}

void FileBrowserModel::update() {
    directoryList->refresh();
}

DirectoryContentsList* FileBrowserModel::getDirectoryList() {
    return directoryList;
}


