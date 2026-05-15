/*
  ==============================================================================

    JavaScriptEditorComponent.h
    Author:  Matthias Pueski

    Tab content for the "JavaScript" view: a juce::CodeEditorComponent on top
    of a CodeDocument with JavaScriptCodeTokeniser, plus toolbar buttons for
    Open / Save / Save As / Run and a TextEditor below for script output.

    Scripts are evaluated with the bundled Duktape engine. A small print() and
    console.log() bridge writes back into the output panel. Errors from
    duk_peval_string() are caught and shown in red.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "JavaScriptCodeTokeniser.h"

class JavaScriptEditorComponent  : public juce::Component,
                                   public juce::Button::Listener
{
public:
    JavaScriptEditorComponent();
    ~JavaScriptEditorComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void buttonClicked (juce::Button*) override;

    void runScript();
    void openFile();
    void saveFile();
    void saveFileAs();
    void newFile();

    void appendOutput (const juce::String& text, juce::Colour colour);

private:
    void writeToFile (const juce::File& f);
    void loadFromFile (const juce::File& f);
    void updateTitle();

    juce::CodeDocument document;
    JavaScriptCodeTokeniser tokeniser;
    std::unique_ptr<juce::CodeEditorComponent> editor;

    juce::TextButton newButton  { "New" };
    juce::TextButton openButton { "Open" };
    juce::TextButton saveButton { "Save" };
    juce::TextButton saveAsButton { "Save As" };
    juce::TextButton runButton  { "Run" };
    juce::TextButton clearButton { "Clear Output" };

    juce::Label titleLabel;
    juce::TextEditor output;

    juce::File currentFile;
    std::unique_ptr<juce::FileChooser> chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JavaScriptEditorComponent)
};
