#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "ModuleBrowserTable.h"

class ModuleBrowser  : public juce::Component,
                       public juce::TextEditor::Listener,
                       public juce::DragAndDropContainer

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
    juce::ScopedPointer<ModuleBrowserTable> table = nullptr;

    juce::ScopedPointer<juce::Label> label;
    juce::ScopedPointer<juce::TextEditor> searchText;
    
    juce::ScopedPointer<juce::ResizableCornerComponent> resizer = nullptr;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModuleBrowser)
};


