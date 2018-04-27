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

//==============================================================================
/*
*/
class ModuleBrowserTable    : public Component, public TableListBoxModel
{
public:
    ModuleBrowserTable()
    {
        table = new TableListBox();
        setSize(580,400);
        table->getHeader().addColumn("Module", 1, getWidth());
        table->setSize(getWidth(), getHeight());
        table->setModel(this);
        addAndMakeVisible(table);
        resized();
        
        std::map<int,PrefabFactory::Prefab> _prefabs = PrefabFactory::getInstance()->getPrefabNames();
        
        for (std::map<int,PrefabFactory::Prefab>::iterator it  = _prefabs.begin();it != _prefabs.end();++it) {
            prefabs.add((*it).second.getName());
        }
    }

    ~ModuleBrowserTable()
    {
        delete table;
    }

    int getNumRows() override {
        return prefabs.size();
    }
    void paintCell (Graphics& g,
                    int rowNumber,
                    int columnId,
                    int width, int height,
                    bool rowIsSelected) override {
        
        g.setColour(juce::Colours::black);
        
        String text = "";
        
        if (columnId == 1) {
            text = prefabs.getReference(rowNumber);
        }
        g.drawText(text, 0,0, width,height, juce::Justification::centredLeft);
    }
    
    void paintRowBackground (Graphics& g,
                             int rowNumber,
                             int width, int height,
                             bool rowIsSelected) override {
        if (rowNumber % 2 == 0) {
            g.setColour(juce::Colours::grey);
        }
        else {
            g.setColour(juce::Colours::lightgrey);
        }
        g.fillRect(0,0,width,height);
    }
    
    void paint (Graphics& g) override
    {
        g.fillAll (Colour (0xff323e44));
    }

    void resized() override
    {
        if (getParentComponent() != nullptr) {
            setSize(getParentWidth(), getParentHeight());
            if (table != nullptr)
                table->setSize(getWidth(), getHeight());

        }

    }

private:
    TableListBox* table = nullptr;
    
    StringArray prefabs;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModuleBrowserTable)
};
