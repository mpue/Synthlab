/*
  ==============================================================================

    LogView.h

    Bottom-tab component that mirrors the SynthlabLogger ring buffer onto
    the screen. Polls at a low rate from the message thread — safe even
    when log calls originate from the audio thread.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "SynthlabLogger.h"

class LogView : public juce::Component,
                private juce::Timer
{
public:
    LogView()
    {
        output.setMultiLine (true, false);
        output.setReadOnly (true);
        output.setScrollbarsShown (true);
        output.setCaretVisible (false);
        output.setPopupMenuEnabled (true);
        output.setFont (juce::Font ("Menlo", 12.0f, juce::Font::plain));
        addAndMakeVisible (output);

        clearBtn.setButtonText ("Clear");
        clearBtn.onClick = [this] { output.clear(); };
        addAndMakeVisible (clearBtn);

        openBtn.setButtonText ("Open log file");
        openBtn.onClick = [] {
            auto f = SynthlabLogger::get().getLogFile();
            if (f.existsAsFile())
                f.revealToUser();
        };
        addAndMakeVisible (openBtn);

        revealDirBtn.setButtonText ("Show folder");
        revealDirBtn.onClick = [] {
            auto d = SynthlabLogger::getLogDir();
            if (d.isDirectory())
                d.revealToUser();
        };
        addAndMakeVisible (revealDirBtn);

        autoScrollBtn.setButtonText ("Auto-scroll");
        autoScrollBtn.setToggleState (true, juce::dontSendNotification);
        autoScrollBtn.setClickingTogglesState (true);
        addAndMakeVisible (autoScrollBtn);

        startTimerHz (8);
    }

    ~LogView() override { stopTimer(); }

    void resized() override
    {
        constexpr int rowH = 26;
        constexpr int gap  = 4;

        auto r = getLocalBounds();
        auto bar = r.removeFromTop (rowH);
        r.removeFromTop (gap);

        autoScrollBtn.setBounds (bar.removeFromLeft (110));
        bar.removeFromLeft (gap);
        clearBtn.setBounds (bar.removeFromLeft (70));
        bar.removeFromLeft (gap);
        openBtn.setBounds (bar.removeFromLeft (110));
        bar.removeFromLeft (gap);
        revealDirBtn.setBounds (bar.removeFromLeft (110));

        output.setBounds (r);
    }

private:
    void timerCallback() override
    {
        auto lines = SynthlabLogger::get().drainSince (seq);
        if (lines.isEmpty())
            return;

        const bool scroll = autoScrollBtn.getToggleState();
        output.moveCaretToEnd();
        for (auto& l : lines)
            output.insertTextAtCaret (l + "\n");

        if (scroll)
            output.moveCaretToEnd();
    }

    juce::TextEditor output;
    juce::TextButton clearBtn, openBtn, revealDirBtn;
    juce::ToggleButton autoScrollBtn;
    juce::int64 seq = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LogView)
};
