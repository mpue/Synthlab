/*
  ==============================================================================

    ToolbarComboBox.h
    Created: 20 Apr 2018 9:38:54am
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"

//==============================================================================
/*
*/
class ToolbarComboBox : public juce::ToolbarItemComponent
{
public:
    ToolbarComboBox (const int toolbarItemId)
    : juce::ToolbarItemComponent (toolbarItemId, "Custom Toolbar Item", false),
    comboBox ("layers")
    {
        addAndMakeVisible (comboBox);
        
        comboBox.addItem("All", 1);
        comboBox.addItem("Controls only", 2);
        comboBox.addItem("Modules", 3);
        comboBox.addItem("Connections", 4);
        
        comboBox.setSelectedId (1);
        comboBox.setEditableText (false);
    }
    
    juce::ComboBox& getComboBox() {
        return comboBox;
    }
    
    bool getToolbarItemSizes (int /*toolbarDepth*/, bool isVertical,
                              int& preferredSize, int& minSize, int& maxSize) override
    {
        if (isVertical)
            return false;
        
        preferredSize = 250;
        minSize = 80;
        maxSize = 300;
        return true;
    }
    
    void paintButtonArea (juce::Graphics&, int, int, bool, bool) override
    {
    }
    
    void contentAreaChanged (const juce::Rectangle<int>& newArea) override
    {
        comboBox.setSize (newArea.getWidth() - 2,
                          juce::jmin (newArea.getHeight() - 2, 22));
        
        comboBox.setCentrePosition (newArea.getCentreX(), newArea.getCentreY());
    }
    
private:
    juce::ComboBox comboBox;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ToolbarComboBox)
};
