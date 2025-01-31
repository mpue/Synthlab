/*
  ==============================================================================

    ShellConsoleComponent.h
    Created: 27 Jan 2025 9:44:14pm
    Author:  mpue

  ==============================================================================
*/

#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
#include "CommandParser.h"

class ShellConsoleComponent : public juce::Component,
    private juce::KeyListener
{
public:
    ShellConsoleComponent(CommandParser& parser) : commandParser(parser)
    {
        addAndMakeVisible(consoleEditor);
        consoleEditor.setMultiLine(true);
        consoleEditor.setReadOnly(false);
        consoleEditor.setScrollbarsShown(true);
        consoleEditor.setCaretVisible(true);
        consoleEditor.setPopupMenuEnabled(false);
        consoleEditor.addKeyListener(this);
        consoleEditor.setFont(juce::Font("Consolas", 16.0f, juce::Font::plain));

        printToConsole("Welcome to Synthlab Shell! Type 'help' for a list of commands.");
        printPrompt();
    }

    ~ShellConsoleComponent() override = default;

    void resized() override
    {
        consoleEditor.setBounds(getLocalBounds());
    }

    void printToConsole(const juce::String& text)
    {
        consoleEditor.moveCaretToEnd();
        consoleEditor.insertTextAtCaret(text + "\n");
    }

    void printPrompt()
    {
        consoleEditor.moveCaretToEnd();
        consoleEditor.insertTextAtCaret("$ ");
        promptPosition = consoleEditor.getText().length();
    }

private:
    CommandParser& commandParser;
    juce::TextEditor consoleEditor;
    juce::StringArray commandHistory;
    int historyIndex = -1;
    int promptPosition = 0;

    bool keyPressed(const juce::KeyPress& key, juce::Component*) override
    {
        if (key == juce::KeyPress::returnKey)
        {
            auto text = consoleEditor.getText();
            auto lastLine = text.substring(promptPosition).trim();
            executeCommand(lastLine);
            return true;
        }
        else if (key == juce::KeyPress::upKey)
        {
            if (historyIndex > 0)
            {
                historyIndex--;
                replaceCurrentLine(commandHistory[historyIndex]);
            }
            return true;
        }
        else if (key == juce::KeyPress::downKey)
        {
            if (historyIndex < commandHistory.size() - 1)
            {
                historyIndex++;
                replaceCurrentLine(commandHistory[historyIndex]);
            }
            else if (historyIndex == commandHistory.size() - 1)
            {
                historyIndex++;
                replaceCurrentLine("");
            }
            return true;
        }
        else if (key == juce::KeyPress::backspaceKey)
        {
            if (consoleEditor.getCaretPosition() <= promptPosition)
                return true;
        }
        else if (key == juce::KeyPress::leftKey)
        {
            if (consoleEditor.getCaretPosition() <= promptPosition)
                return true;
        }

        return false;
    }

    void executeCommand(const juce::String& input)
    {
        if (input.isEmpty())
        {
            printPrompt();
            return;
        }

        commandHistory.add(input);
        historyIndex = commandHistory.size();

        std::string result = commandParser.parseAndExecute(input.toStdString());

        // Ensure we print something even if the result is empty
        if (result.empty()) {
            printToConsole("(No output)");
        }
        else {
            printToConsole(result);
        }

        printPrompt();
    }

    void replaceCurrentLine(const juce::String& newText)
    {
        auto text = consoleEditor.getText();
        consoleEditor.setText(text.substring(0, promptPosition) + newText, false);
        consoleEditor.moveCaretToEnd();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ShellConsoleComponent)
};
