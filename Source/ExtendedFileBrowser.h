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

#include <vector>

class ExtendedFileBrowser;

class FileBrowserModel : public juce::TableListBoxModel {
public:

    FileBrowserModel(juce::DirectoryContentsList* directoryList, juce::File& initalDir);

    virtual void paintRowBackground (juce::Graphics& g,
                                     int rowNumber,
                                     int width, int height,
                                     bool rowIsSelected) override;
    virtual void paintCell (juce::Graphics& g,
                            int rowNumber,
                            int columnId,
                            int width, int height,
                            bool rowIsSelected) override;
    virtual int getNumRows() override;
    void setCurrentDir(juce::File* dir);
    void update();
    void rebuildSorted();
    juce::File getFileForRow(int row) const;
    juce::DirectoryContentsList* getDirectoryList();

    juce::String& getCurrentDir() {
        return currentDirectory;
    }

    juce::var getDragSourceDescription (const juce::SparseSet<int>& currentlySelectedRows) override{

        if (!currentlySelectedRows.getTotalRange().isEmpty()) {
            int row = currentlySelectedRows.getTotalRange().getStart();
            juce::File f = getFileForRow(row);
            if (f.exists())
                return f.getFullPathName();
        }

        return juce::var();
    }

private:
    juce::DirectoryContentsList* directoryList;
    juce::String currentDirectory;
    std::vector<juce::File> sortedFiles;
};

class FileTreeItem : public juce::TreeViewItem {
public:
    FileTreeItem(const juce::File& file, ExtendedFileBrowser* owner);

    bool mightContainSubItems() override;
    juce::String getUniqueName() const override;
    void itemOpennessChanged(bool isNowOpen) override;
    void paintItem(juce::Graphics& g, int width, int height) override;
    void itemSelectionChanged(bool isNowSelected) override;
    void itemDoubleClicked(const juce::MouseEvent& e) override;
    int getItemHeight() const override { return 20; }
    juce::var getDragSourceDescription() override;

    juce::File getFile() const { return file; }

private:
    void loadChildren();

    juce::File file;
    ExtendedFileBrowser* owner;
};

class ExtendedFileBrowser : public juce::Component,
                            public juce::ChangeListener,
                            public juce::Timer,
                            public ChangeBroadcaster,
                            public Button::Listener,
                            public DragAndDropContainer
{


public:
    enum class Mode { Flat, Tree };

    ExtendedFileBrowser(const juce::File& initialFileOrDirectory,const juce::WildcardFileFilter* fileFilter, FileBrowserModel* model);
    ~ExtendedFileBrowser();

    void mouseDrag (const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void paint (juce::Graphics& g) override;
    void resized() override;
    virtual void changeListenerCallback (juce::ChangeBroadcaster* source) override;
    File* getSelectedFile();
    void saveState();
    void loadState();

    void focusLost(FocusChangeType cause) override;

    juce::TableListBox* getTable() {
        return table;
    };

    FileBrowserModel* getModel() {
        return model;
    }

    void buttonClicked(Button* button) override;

    virtual void timerCallback() override;

    void setMode(Mode newMode);
    Mode getMode() const { return mode; }

    void treeItemSelected(const juce::File& file);
    void treeItemDoubleClicked(const juce::File& file);

private:
    void rebuildTree();

    FileBrowserModel* model = nullptr;
    const juce::File& initialDir;
    const juce::WildcardFileFilter* filter;
    File* selectedFile = nullptr;
    juce::TableListBox* table = nullptr;
    juce::Viewport* view = nullptr;
    juce::TreeView* tree = nullptr;
    FileTreeItem* treeRoot = nullptr;
    juce::TextButton* modeButton = nullptr;
    Mode mode = Mode::Flat;
    Sampler* sampler;
    bool playing = false;
    std::vector<Button*> driveButtons;
};

