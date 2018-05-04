#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "ModuleBrowserTable.h"

class ModuleBrowser  : public juce::Component,
                       public juce::TextEditor::Listener

{
public:

    ModuleBrowser ();
    ~ModuleBrowser();

    void paint (juce::Graphics& g) override;
    void resized() override;
    void textEditorTextChanged (juce::TextEditor&) override;
    
    ModuleBrowserTable* getTable() {
        return table;
    };
    
    
private:

    juce::DropShadow* shadow = nullptr;
    juce::DropShadower* shadower = nullptr;
    ModuleBrowserTable* table = nullptr;

    juce::ScopedPointer<juce::Label> label;
    juce::ScopedPointer<juce::TextEditor> searchText;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModuleBrowser)
};


