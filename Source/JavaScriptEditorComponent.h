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

class TrackNavigator;
class SynthEditor;

// Defined out-of-line in the .cpp — forward-declared here so we can hold
// unique_ptr members without exposing the autocomplete internals.
class JavaScriptCodeEditor;
class AutoCompletePopup;

class JavaScriptEditorComponent  : public juce::Component,
                                   public juce::Button::Listener
{
public:
    JavaScriptEditorComponent();
    ~JavaScriptEditorComponent() override;

    // Wired by MainComponent when the JavaScript tab is created. The script
    // bridge calls play/stop/position on this pointer; the editor itself does
    // not own the navigator and must not outlive it.
    void setTransportNavigator (TrackNavigator* nav) { transportNavigator = nav; }
    TrackNavigator* getTransportNavigator() const    { return transportNavigator; }

    // Required for synthlab.actions.addAudioTrack / addMidiTrack / deleteTrack
    // — AddTrackAction and TrackDeleteAction both take the SynthEditor.
    void setSynthEditor (SynthEditor* ed) { synthEditor = ed; }
    SynthEditor* getSynthEditor() const   { return synthEditor; }

    void paint (juce::Graphics&) override;
    void resized() override;

    void buttonClicked (juce::Button*) override;

    void runScript();
    // Run an arbitrary script string in the same Duktape pipeline used by the
    // editor's Run button — output and errors land in the same panel. `label`
    // is shown in the "--- Running ... ---" banner so macros can be told apart
    // from editor-buffer runs.
    void runScriptText (const juce::String& script, const juce::String& label);
    // Read the file and call runScriptText with the file name as the label.
    void runFile (const juce::File& file);

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
    std::unique_ptr<JavaScriptCodeEditor> editor;
    std::unique_ptr<AutoCompletePopup> autoComplete;

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

    TrackNavigator* transportNavigator = nullptr;
    SynthEditor*    synthEditor        = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JavaScriptEditorComponent)
};
