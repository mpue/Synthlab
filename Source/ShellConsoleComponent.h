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
    private juce::TextEditor::Listener,
    private juce::Button::Listener
{
public:
    ShellConsoleComponent(CommandParser& parser) : commandParser(parser)
    {
        clearButton.setButtonText("Clear");
        clearButton.addListener(this);
        addAndMakeVisible(clearButton);

        // Output: schreibgeschützt, scrollbar, Verlauf von Befehlen + Antworten.
        addAndMakeVisible(output);
        output.setMultiLine(true, false);
        output.setReadOnly(true);
        output.setScrollbarsShown(true);
        output.setCaretVisible(false);
        output.setPopupMenuEnabled(true);
        output.setFont(juce::Font("Consolas", 14.0f, juce::Font::plain));

        // Eigene einzeilige Eingabezeile am unteren Rand — bleibt unabhängig
        // vom Output-Scrollzustand immer sichtbar, damit beim Tippen kein
        // Versteckspiel mit der Scroll-Position entsteht.
        addAndMakeVisible(input);
        input.setMultiLine(false);
        input.setReturnKeyStartsNewLine(false);
        input.setScrollbarsShown(false);
        input.setCaretVisible(true);
        input.setPopupMenuEnabled(true);
        input.setFont(juce::Font("Consolas", 14.0f, juce::Font::plain));
        input.addListener(this);
        input.onArrowUp   = [this] { showHistoryPrev(); };
        input.onArrowDown = [this] { showHistoryNext(); };

        promptLabel.setText("$", juce::dontSendNotification);
        promptLabel.setFont(juce::Font("Consolas", 14.0f, juce::Font::bold));
        promptLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(promptLabel);

        appendLine("Welcome to Synthlab Shell! Type 'help' for a list of commands.");
    }

    ~ShellConsoleComponent() override
    {
        input.removeListener(this);
        clearButton.removeListener(this);
    }

    void resized() override
    {
        // Eingabezeile oben (statt unten) — der Bottom-Tab wird vom Parent
        // teilweise unten abgeschnitten, oben ist sie immer sichtbar.
        constexpr int rowH    = 26;
        constexpr int promptW = 18;
        constexpr int clearW  = 70;
        constexpr int gap     = 4;

        auto r = getLocalBounds();
        auto topRow = r.removeFromTop(rowH);
        r.removeFromTop(gap);

        promptLabel.setBounds(topRow.removeFromLeft(promptW));
        clearButton.setBounds(topRow.removeFromRight(clearW));
        input.setBounds(topRow.reduced(0, 1));
        output.setBounds(r);
    }

    void appendLine(const juce::String& text)
    {
        output.moveCaretToEnd();
        output.insertTextAtCaret(text + "\n");
        output.moveCaretToEnd();
    }

    void clearConsole()
    {
        output.clear();
        appendLine("Welcome to Synthlab Shell! Type 'help' for a list of commands.");
        input.grabKeyboardFocus();
    }

private:
    // TextEditor mit zusätzlichen Hooks für Up/Down — JUCEs Listener-Interface
    // hat dafür keinen Callback, also leiten wir es minimal um.
    class HistoryInput : public juce::TextEditor
    {
    public:
        std::function<void()> onArrowUp;
        std::function<void()> onArrowDown;

        bool keyPressed(const juce::KeyPress& key) override
        {
            if (key == juce::KeyPress::upKey   && onArrowUp)   { onArrowUp();   return true; }
            if (key == juce::KeyPress::downKey && onArrowDown) { onArrowDown(); return true; }
            return juce::TextEditor::keyPressed(key);
        }
    };

    CommandParser& commandParser;

    juce::TextEditor output;
    HistoryInput input;
    juce::Label promptLabel;
    juce::TextButton clearButton;

    juce::StringArray commandHistory;
    int historyIndex = -1;

    void textEditorReturnKeyPressed(juce::TextEditor& ed) override
    {
        if (&ed != &input)
            return;
        executeCommand(input.getText().trim());
        input.clear();
    }

    void buttonClicked(juce::Button* b) override
    {
        if (b == &clearButton)
            clearConsole();
    }

    void showHistoryPrev()
    {
        if (commandHistory.isEmpty()) return;
        historyIndex = juce::jmax(0, historyIndex - 1);
        input.setText(commandHistory[historyIndex], juce::dontSendNotification);
        input.moveCaretToEnd();
    }

    void showHistoryNext()
    {
        if (commandHistory.isEmpty()) return;
        if (historyIndex < commandHistory.size() - 1)
        {
            ++historyIndex;
            input.setText(commandHistory[historyIndex], juce::dontSendNotification);
        }
        else
        {
            historyIndex = commandHistory.size();
            input.clear();
        }
        input.moveCaretToEnd();
    }

    void executeCommand(const juce::String& cmd)
    {
        if (cmd.isEmpty()) return;

        commandHistory.add(cmd);
        historyIndex = commandHistory.size();

        appendLine("$ " + cmd);

        std::string result = commandParser.parseAndExecute(cmd.toStdString());
        if (result.empty())
            appendLine("(No output)");
        else
            appendLine(juce::String(result));
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ShellConsoleComponent)
};
