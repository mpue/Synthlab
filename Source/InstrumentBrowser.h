/*
  ==============================================================================

    InstrumentBrowser.h

    Lightweight component that lists every scanned virtual instrument plugin
    and emits a callback on double-click. Includes a filter field at the top
    for fast incremental search. Used as a tab next to the file browser so
    the user can create an Instrument track without going through the plugin
    chooser dialog.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Plugins/PluginManager.h"

class InstrumentBrowser : public juce::Component,
                          public juce::ListBoxModel
{
public:
    InstrumentBrowser()
    {
        // Filter field — incremental case-insensitive substring match.
        filter.setTextToShowWhenEmpty("Filter...", juce::Colour(0xff888888));
        filter.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff2a2a2a));
        filter.setColour(juce::TextEditor::textColourId,       juce::Colour(0xffe8e8e8));
        filter.setColour(juce::TextEditor::outlineColourId,    juce::Colour(0xff404040));
        filter.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0xff4d9eff));
        filter.onTextChange = [this]() { applyFilter(); };
        // Esc clears the filter — quick way to reset the view.
        filter.onEscapeKey  = [this]() { filter.clear(); applyFilter(); };
        // Return: if exactly one match, treat it like a double-click.
        filter.onReturnKey  = [this]() {
            if (visibleNames.size() == 1 && onInstrumentDoubleClicked)
                onInstrumentDoubleClicked(visibleNames[0]);
        };
        addAndMakeVisible(filter);

        list.setModel(this);
        list.setRowHeight(22);
        list.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff1f1f1f));
        addAndMakeVisible(list);
        refresh();
    }

    /** Re-reads the available-plugin list from PluginManager. Call after a
        rescan or when the tab becomes visible — the underlying vector in
        PluginManager is filled lazily and may grow after this object is
        constructed. */
    void refresh()
    {
        allNames.clear();
        if (auto* pm = PluginManager::getInstance())
        {
            // Use getAvailablePlugins() (the live vector) rather than
            // getPlugins() — the latter caches an internal StringArray on its
            // first call, so any later additions wouldn't show up here.
            const auto& plugins = pm->getAvailablePlugins();
            for (const auto& n : plugins)
                allNames.add(n);
            allNames.sort(true);
        }
        applyFilter();
    }

    /** Called with the plugin name on double-click. The owner wires this to
        SynthEditor::addVirtualInstrumentTrack(name). */
    std::function<void(const juce::String&)> onInstrumentDoubleClicked;

    // -- Component ---------------------------------------------------------
    void resized() override
    {
        auto r = getLocalBounds();
        const int filterRowH = 26;
        filter.setBounds(r.removeFromTop(filterRowH).reduced(4, 3));
        list  .setBounds(r);
    }

    void paint(juce::Graphics& g) override { g.fillAll(juce::Colour(0xff1f1f1f)); }

    // Refresh whenever the tab actually becomes visible to the user, so a
    // plugin rescan after this component was created shows up automatically.
    void visibilityChanged() override
    {
        if (isShowing())
            refresh();
    }

    // -- ListBoxModel ------------------------------------------------------
    int getNumRows() override { return visibleNames.size(); }

    void paintListBoxItem(int rowNumber, juce::Graphics& g,
                          int width, int height, bool rowIsSelected) override
    {
        if (rowNumber < 0 || rowNumber >= visibleNames.size())
            return;

        if (rowIsSelected)
            g.fillAll(juce::Colour(0xff4d9eff).withAlpha(0.35f));
        else if (rowNumber % 2 == 1)
            g.fillAll(juce::Colour(0xff262626));

        g.setColour(juce::Colour(0xffe8e8e8));
        g.setFont(juce::Font(13.0f));
        g.drawText(visibleNames[rowNumber], 8, 0, width - 16, height,
                   juce::Justification::centredLeft, true);
    }

    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override
    {
        if (row < 0 || row >= visibleNames.size()) return;
        if (onInstrumentDoubleClicked)
            onInstrumentDoubleClicked(visibleNames[row]);
    }

private:
    /** Recompute the displayed list based on the current filter text. */
    void applyFilter()
    {
        const juce::String q = filter.getText().trim();
        visibleNames.clear();

        if (q.isEmpty())
        {
            visibleNames = allNames;
        }
        else
        {
            for (const auto& n : allNames)
                if (n.containsIgnoreCase(q))
                    visibleNames.add(n);
        }

        list.updateContent();
        list.repaint();
    }

    // Pass nullptr here and set the model in the body — at the time this
    // field's initializer runs, `this` isn't fully constructed yet.
    juce::TextEditor filter { "InstrumentFilter" };
    juce::ListBox    list   { "InstrumentBrowserList", nullptr };
    juce::StringArray allNames;     // full unfiltered list
    juce::StringArray visibleNames; // currently displayed (filtered) list

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InstrumentBrowser)
};
