//
//  ExtendedFileBrowser.cpp
//  Synthlab - App
//
//  Created by Matthias Pueski on 30.04.18.
//  Copyright © 2018 Pueski. All rights reserved.
//

#include "ExtendedFileBrowser.h"
#include "../JuceLibraryCode/JuceHeader.h"
#include "Project/Project.h"
#include "AudioManager.h"

#include <algorithm>

using juce::File;
using juce::WildcardFileFilter;
using juce::TableListBox;
using juce::TreeView;
using juce::Viewport;
using juce::Colour;
using juce::String;
using juce::DirectoryContentsList;
using juce::ChangeBroadcaster;
using juce::Graphics;
using juce::ValueTree;
using juce::OutputStream;
using juce::XmlElement;
using juce::XmlDocument;
using juce::ScopedPointer;

ExtendedFileBrowser::ExtendedFileBrowser(const File& initialFileOrDirectory,const WildcardFileFilter* fileFilter, FileBrowserModel* model) : initialDir(initialFileOrDirectory) {

    // addMouseListener(this, true);
    table = new TableListBox();
    table->getHeader().addColumn("File", 1, 350);
    table->getHeader().addColumn("Size", 2, 50);
    table->setModel(model);

    juce::Array<juce::File> drives;

    File::findFileSystemRoots(drives);

    for (int i = 0; i < drives.size(); i++) {
        juce::File f = drives.getReference(i);
        TextButton* button = new TextButton(f.getFileName());
        button->setSize(30, 20) ;
        button->setTopLeftPosition(i * 35, 0);
        driveButtons.push_back(button);
        addAndMakeVisible(button);
        button->addListener(this);
    }

    TextButton* stopButton = new TextButton("STOP");
    stopButton->setSize(35, 20);
    stopButton->setTopLeftPosition(drives.size() * 35, 0);
    driveButtons.push_back(stopButton);
    addAndMakeVisible(stopButton);
    stopButton->addListener(this);

    modeButton = new juce::TextButton("Tree");
    modeButton->setSize(50, 20);
    modeButton->setTopLeftPosition((int)drives.size() * 35 + 40, 0);
    addAndMakeVisible(modeButton);
    modeButton->addListener(this);

    this->model = model;
    view  = new Viewport();
    view->setTopLeftPosition(0,25);
    view->setViewedComponent(table, false);
    addAndMakeVisible(view);

    tree = new TreeView();
    tree->setColour(TreeView::backgroundColourId, Colour(0xff1a1a1a));
    tree->setDefaultOpenness(false);
    tree->setRootItemVisible(true);
    addChildComponent(tree);

    sampler = Project::getInstance()->getDefaultSampler();
    table->addMouseListener(this, true);

    // Filter-Eingabezeile unten links — substring-match (case-insensitive).
    // Greift sowohl im Flat- als auch im Tree-Mode. Verzeichnisse bleiben
    // immer sichtbar, damit der Nutzer noch navigieren kann.
    filterLabel = new juce::Label({}, "Filter");
    filterLabel->setColour(juce::Label::textColourId, juce::Colours::white);
    filterLabel->setFont(juce::Font(11.0f));
    addAndMakeVisible(filterLabel);

    filterEditor = new juce::TextEditor();
    filterEditor->setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff2a2a2a));
    filterEditor->setColour(juce::TextEditor::textColourId, juce::Colours::white);
    filterEditor->setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff4d9eff).withAlpha(0.5f));
    filterEditor->setTextToShowWhenEmpty("type to filter samples...", juce::Colours::grey);
    filterEditor->onTextChange = [this]() {
        filterText = filterEditor->getText().trim();
        applyFilter();
    };
    filterEditor->onEscapeKey = [this]() {
        filterEditor->setText({}, juce::dontSendNotification);
        filterText.clear();
        applyFilter();
    };
    addAndMakeVisible(filterEditor);

    loadState();

    model->update();
    model->rebuildSorted();
    table->updateContent();
	setWantsKeyboardFocus(true);

    repaint();
}

ExtendedFileBrowser::~ExtendedFileBrowser() {
    saveState();
    if (tree != nullptr) {
        tree->setRootItem(nullptr);
    }
    delete treeRoot;
    delete tree;
    delete table;
    delete view;
    for (int i = 0; i < driveButtons.size(); i++) {
        delete driveButtons.at(i);
    }
    delete modeButton;
    delete filterEditor;
    delete filterLabel;
}

void ExtendedFileBrowser::mouseDrag(const juce::MouseEvent &event) {
    /*
    var description;

    // setDragImageForIndex(0, ImageCache::getFromMemory(BinaryData::module_png, BinaryData::module_pngSize));
    startDragging(description,table);
     */

}

void ExtendedFileBrowser::paint(juce::Graphics &g) {
    g.fillAll (Colour (0xff1a1a1a));
    // Component::paint(g);
}

void ExtendedFileBrowser::resized() {
    if (getParentComponent() != nullptr) {
        setSize(getParentWidth(), getParentHeight());
    }

    // Layout:  Buttons-Reihe (y=0..20) | Filter-Reihe (y=25..47) | Tabelle/Tree
    // Der Filter liegt direkt unterhalb der Drive/Mode-Buttons. Die
    // Tabelle/der Tree starten dahinter.
    const int filterY = 25;
    const int filterH = 22;

    if (filterLabel != nullptr)
        filterLabel->setBounds(4, filterY, 36, filterH);
    if (filterEditor != nullptr)
        filterEditor->setBounds(40, filterY, juce::jmax(40, getWidth() - 44), filterH);

    const int contentTop = filterY + filterH + 4;
    const int contentH   = juce::jmax(0, getHeight() - contentTop);

    view->setBounds(0, contentTop, getWidth(), contentH);
    table->setSize(getWidth(), juce::jmax(0, contentH - 5));

    if (tree != nullptr)
        tree->setBounds(0, contentTop, getWidth(), contentH);
}

void ExtendedFileBrowser::applyFilter() {
    if (model != nullptr) {
        model->setFilter(filterText);
        model->rebuildSorted();
    }
    if (table != nullptr) {
        table->updateContent();
        table->repaint();
    }

    const bool filterActive = filterText.isNotEmpty();

    // Wenn ein Filter aktiv ist, blenden wir den Tree aus und zeigen die
    // flache Trefferliste in der Tabelle — egal welchen Mode der Nutzer
    // sonst eingestellt hat. Sobald der Filter wieder leer ist, kehrt der
    // urspruengliche Mode zurueck.
    if (filterActive) {
        if (tree != nullptr) tree->setVisible(false);
        if (view != nullptr) view->setVisible(true);
    } else {
        if (mode == Mode::Tree) {
            if (view != nullptr) view->setVisible(false);
            if (tree != nullptr) {
                tree->setVisible(true);
                rebuildTree(); // Tree neu aus aktuellem Verzeichnis aufbauen
            }
        } else {
            if (tree != nullptr) tree->setVisible(false);
            if (view != nullptr) view->setVisible(true);
        }
    }
}

void ExtendedFileBrowser::changeListenerCallback (ChangeBroadcaster* source) {
    if (model != nullptr)
        model->rebuildSorted();
    if (table != nullptr)
        table->updateContent();
    if (mode == Mode::Tree)
        rebuildTree();
}

void ExtendedFileBrowser::mouseDown(const juce::MouseEvent &event) {

	grabKeyboardFocus();

    if (mode != Mode::Flat)
        return;

    if (table->getSelectedRow() > 0) {
        File f = model->getFileForRow(table->getSelectedRow());
        if (f.exists()) {
            if (!f.isDirectory()) {
                if (!f.getFileNameWithoutExtension().startsWith(".")) {
                    auto ext = f.getFileExtension().toLowerCase();
                    if (ext.contains("wav") ||
                        ext.contains("mp3") ||
                        ext.contains("aif") ||
                        ext.contains("ogg")) {
                         Project::getInstance()->getDefaultSampler()->stop();
                        Project::getInstance()->getDefaultSampler()->loadSample(f);
                        Project::getInstance()->getDefaultSampler()->play();
                    }
                }
            }
        }
    }

}

void ExtendedFileBrowser::mouseDoubleClick(const juce::MouseEvent &event) {

    if (mode != Mode::Flat)
        return;

    if (table->getSelectedRow() > 0) {
        File f = model->getFileForRow(table->getSelectedRow());
        if (f.exists()) {
            if (f.isDirectory()) {
                model->setCurrentDir(&f);
            }
            else {
                if (selectedFile != nullptr) delete selectedFile;
                selectedFile = new File(f);
                sendChangeMessage();
            }
        }
    }
    else {
        File current = File(model->getCurrentDir());
        File parent = current.getParentDirectory();
        model->setCurrentDir(&parent);
    }

}

void ExtendedFileBrowser::buttonClicked(Button* button)
{
    if (button == modeButton) {
        setMode(mode == Mode::Flat ? Mode::Tree : Mode::Flat);
        return;
    }

    if (button->getButtonText() == "STOP") {
        Project::getInstance()->getDefaultSampler()->stop();
    }
    else {
        juce::File* file = new juce::File(button->getButtonText() + "\\");
        model->setCurrentDir(file);
        delete file;
    }

}

void ExtendedFileBrowser::timerCallback() {

}

void ExtendedFileBrowser::setMode(Mode newMode) {
    mode = newMode;

    if (mode == Mode::Tree) {
        rebuildTree();
        view->setVisible(false);
        tree->setVisible(true);
        modeButton->setButtonText("Flat");
    }
    else {
        if (tree != nullptr)
            tree->setVisible(false);
        view->setVisible(true);
        modeButton->setButtonText("Tree");
    }

    resized();
}

void ExtendedFileBrowser::rebuildTree() {
    if (tree == nullptr)
        return;

    tree->setRootItem(nullptr);
    delete treeRoot;
    treeRoot = nullptr;

    File rootDir(model->getCurrentDir());
    if (!rootDir.exists())
        rootDir = File::getSpecialLocation(File::userHomeDirectory);

    treeRoot = new FileTreeItem(rootDir, this);
    tree->setRootItem(treeRoot);
    treeRoot->setOpen(true);
}

void ExtendedFileBrowser::treeItemSelected(const juce::File& file) {
    if (file.isDirectory())
        return;

    if (file.getFileNameWithoutExtension().startsWith("."))
        return;

    auto ext = file.getFileExtension().toLowerCase();
    if (ext.contains("wav") || ext.contains("mp3") ||
        ext.contains("aif") || ext.contains("ogg")) {
        Project::getInstance()->getDefaultSampler()->stop();
        juce::File copy = file;
        Project::getInstance()->getDefaultSampler()->loadSample(copy);
        Project::getInstance()->getDefaultSampler()->play();
    }
}

void ExtendedFileBrowser::treeItemDoubleClicked(const juce::File& file) {
    if (file.isDirectory())
        return;

    if (selectedFile != nullptr) delete selectedFile;
    selectedFile = new juce::File(file);
    sendChangeMessage();
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
    return (int)sortedFiles.size() + 1;
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
            File f = getFileForRow(rowNumber);
            text = f.getFileName();
            if (f.isDirectory())
                text = "[" + text + "]";
        }
        else {
            text = "[..]";
        }
        g.setColour(juce::Colours::white);
        g.drawText(text, 0,0, width,height, juce::Justification::centredLeft);

    }
    else if (columnId == 2) {
        if (rowNumber > 0) {
            File f = getFileForRow(rowNumber);
            if (f.isDirectory())
                text = "";
            else
                text = String(f.getSize() / 1024) + "kB";
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
        g.setColour(Colour(0xff4d9eff));
    }
    else {
        g.setColour(Colour (0xff1a1a1a));
    }

    g.fillRect(0,0,width,height);
}

void FileBrowserModel::setCurrentDir(juce::File* dir){
    this->currentDirectory = dir->getFullPathName();
    directoryList->setDirectory(*dir, true, true);
    // Verzeichnis gewechselt -> rekursiver Audio-Cache nicht mehr gueltig.
    invalidateRecursiveCache();
}

bool FileBrowserModel::isAudioFile(const juce::File& f) {
    if (f.isDirectory()) return false;
    auto ext = f.getFileExtension().toLowerCase();
    if (ext.isEmpty()) return false;
    // Hand-gepflegte Whitelist statt einer breiten Suche — was hier nicht
    // dabei ist, kann der Default-Sampler ohnehin nicht laden.
    return ext == ".wav" || ext == ".aif" || ext == ".aiff"
        || ext == ".mp3" || ext == ".ogg" || ext == ".flac";
}

void FileBrowserModel::ensureRecursiveAudioCache() {
    if (recursiveCacheValid && recursiveCacheDir == currentDirectory)
        return;

    recursiveAudioCache.clear();
    recursiveCacheDir = currentDirectory;

    juce::File root(currentDirectory);
    if (root.isDirectory()) {
        for (juce::DirectoryEntry entry : juce::RangedDirectoryIterator(
                 root, /*recursive*/ true, "*", juce::File::findFiles)) {
            const juce::File f = entry.getFile();
            if (isAudioFile(f))
                recursiveAudioCache.push_back(f);
        }
    }
    recursiveCacheValid = true;
}

void FileBrowserModel::update() {
    directoryList->refresh();
}

void FileBrowserModel::rebuildSorted() {
    sortedFiles.clear();

    auto cmp = [](const juce::File& a, const juce::File& b) {
        return a.getFileName().compareIgnoreCase(b.getFileName()) < 0;
    };

    if (filterText.isNotEmpty()) {
        // Mit aktivem Filter: rekursive Suche unter currentDirectory und
        // flache Trefferliste — auf Audio-Formate beschraenkt. Den
        // rekursiven Walk machen wir nur EINMAL pro Verzeichnis (Cache),
        // jeder weitere Tastendruck filtert nur noch im Cache.
        ensureRecursiveAudioCache();
        sortedFiles.reserve(recursiveAudioCache.size());
        for (const auto& f : recursiveAudioCache)
            if (f.getFileName().containsIgnoreCase(filterText))
                sortedFiles.push_back(f);
        std::sort(sortedFiles.begin(), sortedFiles.end(), cmp);
        return;
    }

    // Kein Filter: normales Verhalten — Verzeichnisse oben, Files unten.
    const int n = directoryList->getNumFiles();
    std::vector<juce::File> dirs;
    std::vector<juce::File> files;
    dirs.reserve(n);
    files.reserve(n);

    for (int i = 0; i < n; ++i) {
        juce::File f = directoryList->getFile(i);
        if (f.isDirectory())
            dirs.push_back(f);
        else
            files.push_back(f);
    }

    std::sort(dirs.begin(), dirs.end(), cmp);
    std::sort(files.begin(), files.end(), cmp);

    sortedFiles.reserve(dirs.size() + files.size());
    sortedFiles.insert(sortedFiles.end(), dirs.begin(), dirs.end());
    sortedFiles.insert(sortedFiles.end(), files.begin(), files.end());
}

juce::File FileBrowserModel::getFileForRow(int row) const {
    if (row > 0 && (size_t)(row - 1) < sortedFiles.size())
        return sortedFiles[(size_t)(row - 1)];
    return {};
}

DirectoryContentsList* FileBrowserModel::getDirectoryList() {
    return directoryList;
}

void ExtendedFileBrowser::saveState(){
    String userHome = File::getSpecialLocation(File::userHomeDirectory).getFullPathName();
    File appDir = File(userHome+"/.Synthlab");

    if (!appDir.exists()) {
        appDir.createDirectory();
    }

    File configFile = File(userHome+"/.Synthlab/state.xml");

    if (!configFile.exists()) {
        configFile.create();
    }
    else {
        configFile.deleteFile();
        configFile = File(userHome+"/.Synthlab/state.xml");
    }

    ValueTree* v = new ValueTree("SavedState");

    ValueTree child = ValueTree("File");
    child.setProperty("lastDirectory", model->getCurrentDir(), nullptr);
    child.setProperty("browserMode", mode == Mode::Tree ? "tree" : "flat", nullptr);
    v->addChild(child, -1, nullptr);

    std::unique_ptr<OutputStream> os = configFile.createOutputStream();
    std::unique_ptr<XmlElement> xml = v->createXml();
    xml->writeToStream(*os, "");

    os = nullptr;
    xml = nullptr;

    delete v;
}

void ExtendedFileBrowser::loadState() {
    String userHome = File::getSpecialLocation(File::userHomeDirectory).getFullPathName();

    File appDir = File(userHome+"/.Synthlab");

    if (!appDir.exists()) {
        appDir.createDirectory();
    }

    File configFile = File(userHome+"/.Synthlab/state.xml");

    if (configFile.exists()) {
        std::unique_ptr<XmlElement> xml = XmlDocument(configFile).getDocumentElement();
        ValueTree v = ValueTree::fromXml(*xml.get());

        if (v.getNumChildren() > 0) {
            String path = v.getChild(0).getProperty("lastDirectory");
            File* file = new File(path);
            model->setCurrentDir(file);
            delete file;

            String savedMode = v.getChild(0).getProperty("browserMode", "flat");
            setMode(savedMode == "tree" ? Mode::Tree : Mode::Flat);
        }
        xml = nullptr;
    }
}

void ExtendedFileBrowser::focusLost(FocusChangeType cause)
{
    Component::focusLost(cause);
    Project::getInstance()->getDefaultSampler()->stop();
}

File* ExtendedFileBrowser::getSelectedFile() {
    return selectedFile;
}

//===========================================================================
// FileTreeItem
//===========================================================================

FileTreeItem::FileTreeItem(const juce::File& f, ExtendedFileBrowser* o)
    : file(f), owner(o) {
}

bool FileTreeItem::mightContainSubItems() {
    return file.isDirectory();
}

juce::String FileTreeItem::getUniqueName() const {
    return file.getFullPathName();
}

void FileTreeItem::itemOpennessChanged(bool isNowOpen) {
    if (isNowOpen) {
        if (getNumSubItems() == 0)
            loadChildren();
    }
    else {
        clearSubItems();
    }
}

void FileTreeItem::loadChildren() {
    if (!file.isDirectory())
        return;

    juce::Array<juce::File> dirs;
    juce::Array<juce::File> files;

    const juce::String filter = (owner != nullptr) ? owner->getFilterText() : juce::String();
    const bool hasFilter = filter.isNotEmpty();

    for (juce::DirectoryEntry entry : juce::RangedDirectoryIterator(file, false, "*",
            juce::File::findFilesAndDirectories)) {
        juce::File child = entry.getFile();
        if (child.isDirectory()) {
            dirs.add(child);
        } else {
            if (hasFilter && !child.getFileName().containsIgnoreCase(filter))
                continue;
            files.add(child);
        }
    }

    auto cmp = [](const juce::File& a, const juce::File& b) {
        return a.getFileName().compareIgnoreCase(b.getFileName()) < 0;
    };
    std::sort(dirs.begin(), dirs.end(), cmp);
    std::sort(files.begin(), files.end(), cmp);

    for (auto& d : dirs)
        addSubItem(new FileTreeItem(d, owner));
    for (auto& f : files)
        addSubItem(new FileTreeItem(f, owner));
}

void FileTreeItem::paintItem(juce::Graphics& g, int width, int height) {
    if (isSelected())
        g.fillAll(juce::Colour(0xff4d9eff));

    g.setColour(juce::Colours::white);

    juce::String name = file.getFileName();
    if (name.isEmpty())
        name = file.getFullPathName();
    if (file.isDirectory())
        name = "[" + name + "]";

    g.drawText(name, 4, 0, width - 4, height, juce::Justification::centredLeft);
}

void FileTreeItem::itemSelectionChanged(bool isNowSelected) {
    if (isNowSelected && owner != nullptr)
        owner->treeItemSelected(file);
}

void FileTreeItem::itemDoubleClicked(const juce::MouseEvent& e) {
    if (file.isDirectory())
        juce::TreeViewItem::itemDoubleClicked(e);
    else if (owner != nullptr)
        owner->treeItemDoubleClicked(file);
}

juce::var FileTreeItem::getDragSourceDescription() {
    return file.getFullPathName();
}
