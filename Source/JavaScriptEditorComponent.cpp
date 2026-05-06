/*
  ==============================================================================

    JavaScriptEditorComponent.cpp
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "JavaScriptEditorComponent.h"

extern "C" {
    #include "duktape/duktape.h"
}

using namespace juce;

namespace
{
    // The print/console.log bridge runs synchronously on the same thread as
    // runScript(), so a thread-local pointer is enough to route output back
    // into the calling editor instance — no locking required.
    thread_local JavaScriptEditorComponent* currentEditor = nullptr;

    String collectAllArgsAsString (duk_context* ctx)
    {
        const int n = duk_get_top (ctx);
        StringArray parts;

        for (int i = 0; i < n; ++i)
        {
            duk_dup (ctx, i);
            parts.add (CharPointer_UTF8 (duk_safe_to_string (ctx, -1)));
            duk_pop (ctx);
        }

        return parts.joinIntoString (" ");
    }

    duk_ret_t native_print (duk_context* ctx)
    {
        if (currentEditor != nullptr)
            currentEditor->appendOutput (collectAllArgsAsString (ctx) + "\n",
                                         Colours::lightgrey);
        return 0;
    }

    duk_ret_t native_console_log (duk_context* ctx)
    {
        if (currentEditor != nullptr)
            currentEditor->appendOutput (collectAllArgsAsString (ctx) + "\n",
                                         Colours::lightgrey);
        return 0;
    }

    duk_ret_t native_console_error (duk_context* ctx)
    {
        if (currentEditor != nullptr)
            currentEditor->appendOutput (collectAllArgsAsString (ctx) + "\n",
                                         Colours::tomato);
        return 0;
    }
}

JavaScriptEditorComponent::JavaScriptEditorComponent()
{
    titleLabel.setText ("Untitled.js", dontSendNotification);
    titleLabel.setColour (Label::textColourId, Colours::lightgrey);
    titleLabel.setFont (Font (14.0f, Font::bold));
    addAndMakeVisible (titleLabel);

    addAndMakeVisible (newButton);
    addAndMakeVisible (openButton);
    addAndMakeVisible (saveButton);
    addAndMakeVisible (saveAsButton);
    addAndMakeVisible (runButton);
    addAndMakeVisible (clearButton);

    newButton   .addListener (this);
    openButton  .addListener (this);
    saveButton  .addListener (this);
    saveAsButton.addListener (this);
    runButton   .addListener (this);
    clearButton .addListener (this);

    editor = std::make_unique<CodeEditorComponent> (document, &tokeniser);
    editor->setColourScheme (tokeniser.getDefaultColourScheme());
    editor->setColour (CodeEditorComponent::backgroundColourId, Colour (0xff1e1e1e));
    editor->setColour (CodeEditorComponent::defaultTextColourId, Colours::white);
    editor->setColour (CodeEditorComponent::lineNumberBackgroundId, Colour (0xff2a2a2a));
    editor->setColour (CodeEditorComponent::lineNumberTextId, Colour (0xff7a7a7a));
    editor->setColour (CodeEditorComponent::highlightColourId, Colour (0xff3a3a55));
    editor->setFont (Font (Font::getDefaultMonospacedFontName(), 14.0f, Font::plain));
    addAndMakeVisible (editor.get());

    output.setMultiLine (true);
    output.setReadOnly (true);
    output.setCaretVisible (false);
    output.setScrollbarsShown (true);
    output.setColour (TextEditor::backgroundColourId, Colour (0xff141414));
    output.setColour (TextEditor::textColourId, Colours::lightgrey);
    output.setColour (TextEditor::outlineColourId, Colour (0xff333333));
    output.setFont (Font (Font::getDefaultMonospacedFontName(), 13.0f, Font::plain));
    addAndMakeVisible (output);

    document.replaceAllContent (
        "// Synthlab JavaScript editor (Duktape)\n"
        "// console.log() and print() write to the output panel below.\n\n"
        "function greet(name) {\n"
        "    return \"Hello, \" + name + \"!\";\n"
        "}\n\n"
        "console.log(greet(\"Synthlab\"));\n"
        "print(\"2 + 2 =\", 2 + 2);\n");
}

JavaScriptEditorComponent::~JavaScriptEditorComponent()
{
    newButton   .removeListener (this);
    openButton  .removeListener (this);
    saveButton  .removeListener (this);
    saveAsButton.removeListener (this);
    runButton   .removeListener (this);
    clearButton .removeListener (this);
}

void JavaScriptEditorComponent::paint (Graphics& g)
{
    g.fillAll (Colour (0xff1b1b25));
}

void JavaScriptEditorComponent::resized()
{
    auto area = getLocalBounds();

    auto toolbar = area.removeFromTop (30);
    toolbar.reduce (4, 4);

    const int buttonWidth = 70;
    newButton   .setBounds (toolbar.removeFromLeft (buttonWidth));
    toolbar.removeFromLeft (4);
    openButton  .setBounds (toolbar.removeFromLeft (buttonWidth));
    toolbar.removeFromLeft (4);
    saveButton  .setBounds (toolbar.removeFromLeft (buttonWidth));
    toolbar.removeFromLeft (4);
    saveAsButton.setBounds (toolbar.removeFromLeft (buttonWidth));
    toolbar.removeFromLeft (12);
    runButton   .setBounds (toolbar.removeFromLeft (buttonWidth));
    toolbar.removeFromLeft (12);
    clearButton .setBounds (toolbar.removeFromLeft (buttonWidth + 30));
    toolbar.removeFromLeft (12);
    titleLabel  .setBounds (toolbar);

    const int outputHeight = jmax (80, area.getHeight() / 4);
    auto outputArea = area.removeFromBottom (outputHeight);
    outputArea.reduce (4, 4);
    output.setBounds (outputArea);

    area.reduce (4, 4);
    if (editor != nullptr)
        editor->setBounds (area);
}

void JavaScriptEditorComponent::buttonClicked (Button* b)
{
    if (b == &newButton)        newFile();
    else if (b == &openButton)  openFile();
    else if (b == &saveButton)  saveFile();
    else if (b == &saveAsButton) saveFileAs();
    else if (b == &runButton)   runScript();
    else if (b == &clearButton) output.clear();
}

void JavaScriptEditorComponent::newFile()
{
    document.replaceAllContent ("");
    currentFile = File();
    updateTitle();
}

void JavaScriptEditorComponent::openFile()
{
    chooser = std::make_unique<FileChooser> ("Open JavaScript file",
                                             File::getSpecialLocation (File::userHomeDirectory),
                                             "*.js");

    auto flags = FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles;

    chooser->launchAsync (flags, [this] (const FileChooser& fc)
    {
        auto f = fc.getResult();
        if (f.existsAsFile())
            loadFromFile (f);
    });
}

void JavaScriptEditorComponent::loadFromFile (const File& f)
{
    document.replaceAllContent (f.loadFileAsString());
    currentFile = f;
    updateTitle();
    appendOutput ("Loaded " + f.getFullPathName() + "\n", Colours::cornflowerblue);
}

void JavaScriptEditorComponent::saveFile()
{
    if (! currentFile.existsAsFile())
    {
        saveFileAs();
        return;
    }

    writeToFile (currentFile);
}

void JavaScriptEditorComponent::saveFileAs()
{
    auto initial = currentFile.existsAsFile()
                       ? currentFile.getParentDirectory()
                       : File::getSpecialLocation (File::userHomeDirectory);

    chooser = std::make_unique<FileChooser> ("Save JavaScript file", initial, "*.js");

    auto flags = FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles
                 | FileBrowserComponent::warnAboutOverwriting;

    chooser->launchAsync (flags, [this] (const FileChooser& fc)
    {
        auto f = fc.getResult();
        if (f != File())
        {
            if (f.getFileExtension().isEmpty())
                f = f.withFileExtension ("js");

            currentFile = f;
            writeToFile (f);
            updateTitle();
        }
    });
}

void JavaScriptEditorComponent::writeToFile (const File& f)
{
    if (f.replaceWithText (document.getAllContent()))
        appendOutput ("Saved " + f.getFullPathName() + "\n", Colours::cornflowerblue);
    else
        appendOutput ("Failed to save " + f.getFullPathName() + "\n", Colours::tomato);
}

void JavaScriptEditorComponent::updateTitle()
{
    titleLabel.setText (currentFile.existsAsFile() ? currentFile.getFileName() : "Untitled.js",
                        dontSendNotification);
}

void JavaScriptEditorComponent::appendOutput (const String& text, Colour colour)
{
    auto append = [this, text, colour]()
    {
        output.setColour (TextEditor::textColourId, colour);
        output.moveCaretToEnd();
        output.insertTextAtCaret (text);
    };

    if (MessageManager::getInstance()->isThisTheMessageThread())
        append();
    else
        MessageManager::callAsync (append);
}

void JavaScriptEditorComponent::runScript()
{
    const String script = document.getAllContent();

    duk_context* ctx = duk_create_heap_default();
    if (ctx == nullptr)
    {
        appendOutput ("Could not create Duktape heap.\n", Colours::tomato);
        return;
    }

    currentEditor = this;

    // global print()
    duk_push_c_function (ctx, native_print, DUK_VARARGS);
    duk_put_global_string (ctx, "print");

    // global console object with .log() and .error()
    duk_push_object (ctx);
    duk_push_c_function (ctx, native_console_log, DUK_VARARGS);
    duk_put_prop_string (ctx, -2, "log");
    duk_push_c_function (ctx, native_console_error, DUK_VARARGS);
    duk_put_prop_string (ctx, -2, "error");
    duk_put_global_string (ctx, "console");

    appendOutput ("--- Running script ---\n", Colours::cornflowerblue);

    if (duk_peval_string (ctx, script.toRawUTF8()) != 0)
    {
        // peval leaves the error on the top of the stack.
        const char* err = duk_safe_to_string (ctx, -1);
        appendOutput (String ("Error: ") + CharPointer_UTF8 (err) + "\n", Colours::tomato);
    }
    else
    {
        // Show the script's return value (if any) — useful for `2 + 2` style
        // expressions where there's no explicit print.
        if (! duk_is_undefined (ctx, -1))
        {
            const char* result = duk_safe_to_string (ctx, -1);
            appendOutput (String ("=> ") + CharPointer_UTF8 (result) + "\n",
                          Colours::lightgreen);
        }
    }

    duk_pop (ctx);
    duk_destroy_heap (ctx);
    currentEditor = nullptr;

    appendOutput ("--- Done ---\n", Colours::cornflowerblue);
}
