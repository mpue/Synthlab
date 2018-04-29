/*
  ==============================================================================

    ModuleBrowserTable.h
    Created: 27 Apr 2018 2:22:55pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PrefabFactory.h"
#include <map>
#include <vector>

//==============================================================================
/*
*/
class ModuleBrowserTable    : public Component, public TableListBoxModel
{
public:
    ModuleBrowserTable()
    {
        table = new TableListBox();
        // setSize(580,400);
        table->getHeader().addColumn("", 1,80);
        table->getHeader().addColumn("", 2,320);
        table->setRowHeight(64);
        table->setModel(this);
        table->setMultipleSelectionEnabled(false);
        addAndMakeVisible(table);
        resized();
        
        std::map<int,PrefabFactory::Prefab> _prefabs = PrefabFactory::getInstance()->getPrefabNames();
        
        filteredPrefabs = new std::vector<PrefabFactory::Prefab>();
        
        for (std::map<int,PrefabFactory::Prefab>::iterator it  = _prefabs.begin();it != _prefabs.end();++it) {
            (*it).second.setId((*it).first);
            prefabs.push_back((*it).second);
            filteredPrefabs->push_back((*it).second);
        }
        table->addMouseListener(this,true);
        
        
    }

    ~ModuleBrowserTable()
    {
        delete filteredPrefabs;
        delete table;
    }
    
    var getDragSourceDescription (const SparseSet<int>& currentlySelectedRows) override{
        return (filteredPrefabs->at(table->getSelectedRow()).getId());
    }

    
    void update(){
        table->updateContent();
    }

    std::vector<PrefabFactory::Prefab>& getPrefabs() {
        return prefabs;
    }

    std::vector<PrefabFactory::Prefab>* getFilteredPrefabs() {
        return filteredPrefabs;
    }
    
    int getNumRows() override {
        if (filteredPrefabs != nullptr)
            return filteredPrefabs->size();
        return 0;
    }
    void paintCell (Graphics& g,
                    int rowNumber,
                    int columnId,
                    int width, int height,
                    bool rowIsSelected) override {
        
        g.setColour(juce::Colours::black);
        
        String text = "";
        
        if (columnId == 2) {
            text = filteredPrefabs->at(rowNumber).getName();
            
            if (rowIsSelected) {
                g.setColour(juce::Colours::white);
            }
            
            g.drawText(text, 0,0, width,height, juce::Justification::centredLeft);
        }
        else if (columnId == 1) {
            g.drawImageAt(PrefabFactory::getInstance()->getImage(filteredPrefabs->at(rowNumber).getId()), 0,0);
        }
        

    }
    
    void paintRowBackground (Graphics& g,
                             int rowNumber,
                             int width, int height,
                             bool rowIsSelected) override {
        if (rowNumber % 2 == 0) {
            g.setColour(juce::Colours::grey);
        }
        else {
            g.setColour(juce::Colours::darkgrey);
        }
        if (rowIsSelected) {
            g.setColour(juce::Colours::orange);
        }
        
        g.fillRect(0,0,width,height);
    }
    
    void paint (Graphics& g) override
    {
        // g.fillAll (Colour (0xff323e44));
    }

    void resized() override
    {
        if (getParentComponent() != nullptr) {
            setSize(getParentWidth(), getParentHeight());
            if (table != nullptr)
                table->setSize(getWidth() - 12 , getHeight() - 90);

        }

    }

private:
    TableListBox* table = nullptr;
    std::vector<PrefabFactory::Prefab> prefabs;
    std::vector<PrefabFactory::Prefab>* filteredPrefabs = nullptr;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModuleBrowserTable)
};
