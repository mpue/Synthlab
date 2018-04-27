#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "ModuleBrowserTable.h"

class ModuleBrowser  : public Component,
                       public Button::Listener,
                       public TextEditor::Listener

{
public:

    ModuleBrowser ();
    ~ModuleBrowser();

    void paint (Graphics& g) override;
    void resized() override;
    void buttonClicked (Button* buttonThatWasClicked) override;
    void textEditorTextChanged (TextEditor&) override;
    
    ModuleBrowserTable* getTable() {
        return table;
    };
    
    
private:

    DropShadow* shadow = nullptr;
    DropShadower* shadower = nullptr;
    ModuleBrowserTable* table = nullptr;

    ScopedPointer<Label> label;
    ScopedPointer<TextEditor> searchText;
    ScopedPointer<TextButton> searchButton;

    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModuleBrowser)
};


