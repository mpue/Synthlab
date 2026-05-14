/*
  ==============================================================================

    AudioSettingsDialog.h
    Styled wrapper around juce::AudioDeviceSelectorComponent — same look and
    feel as PluginManagerDialog so the two configuration screens match.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

class GeneralSettingsPanel;

class AudioSettingsDialog : public juce::Component,
                            private juce::ChangeListener,
                            private juce::Button::Listener
{
public:
    AudioSettingsDialog(juce::AudioDeviceManager& deviceManager);
    ~AudioSettingsDialog() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    static juce::DialogWindow* show(juce::AudioDeviceManager& deviceManager,
                                    juce::LookAndFeel* lf);

private:
    void changeListenerCallback(juce::ChangeBroadcaster*) override;
    void buttonClicked(juce::Button*) override;

    void paintHeader(juce::Graphics&, juce::Rectangle<int> area);
    juce::String currentDeviceSummary() const;

    juce::AudioDeviceManager& deviceManager;

    std::unique_ptr<juce::AudioDeviceSelectorComponent> selector;
    std::unique_ptr<GeneralSettingsPanel> generalPanel;
    std::unique_ptr<juce::TabbedComponent> tabs;
    std::unique_ptr<juce::TextButton> closeButton;

    juce::Rectangle<int> headerBounds;
    juce::Rectangle<int> selectorBounds;
    juce::Rectangle<int> footerBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioSettingsDialog)
};
