/*
  ==============================================================================

    PluginManagerDialog.h
    Styled wrapper around juce::PluginListComponent — gives the plugin manager
    a hero header, count badge, and a footer that matches the rest of the
    Synthlab UI instead of the bare JUCE default.

  ==============================================================================
*/

#pragma once

#include "../../JuceLibraryCode/JuceHeader.h"

class PluginManagerDialog : public juce::Component,
                            private juce::ChangeListener,
                            private juce::Button::Listener
{
public:
    PluginManagerDialog(juce::AudioPluginFormatManager& apfm,
                        juce::KnownPluginList& list,
                        juce::PropertiesFile* props,
                        const juce::File& deadMansPedalFile);
    ~PluginManagerDialog() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    // Convenience: show as a non-modal dialog window themed to match the
    // surrounding UI. Returns the spawned window so the caller can keep a
    // reference if needed.
    static juce::DialogWindow* show(juce::AudioPluginFormatManager& apfm,
                                    juce::KnownPluginList& list,
                                    juce::PropertiesFile* props,
                                    juce::LookAndFeel* lf,
                                    const juce::File& deadMansPedalFile);

private:
    void changeListenerCallback(juce::ChangeBroadcaster*) override;
    void buttonClicked(juce::Button*) override;

    void paintHeader(juce::Graphics&, juce::Rectangle<int> area);

    juce::KnownPluginList& pluginList;

    std::unique_ptr<juce::PluginListComponent> listComponent;
    std::unique_ptr<juce::TextButton> closeButton;

    juce::Rectangle<int> headerBounds;
    juce::Rectangle<int> listBounds;
    juce::Rectangle<int> footerBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginManagerDialog)
};
