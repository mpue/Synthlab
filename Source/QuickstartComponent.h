/*
  ==============================================================================

    QuickstartComponent.h
    Welcome / onboarding screen — opens as a tab in the editor area and walks
    the user through the three first steps required to get going.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include <functional>

class QuickstartComponent : public juce::Component,
                            private juce::Timer,
                            private juce::Button::Listener
{
public:
    using ActionCallback = std::function<void()>;
    using StatusProvider = std::function<bool()>;

    QuickstartComponent();
    ~QuickstartComponent() override;

    void setOnConfigureAudio(ActionCallback cb)         { onConfigureAudio = std::move(cb); }
    void setOnScanPlugins(ActionCallback cb)            { onScanPlugins = std::move(cb); }
    void setOnCreateInstrumentTrack(ActionCallback cb)  { onCreateInstrumentTrack = std::move(cb); }
    void setOnOpenExistingTrack(ActionCallback cb)      { onOpenExistingTrack = std::move(cb); }

    // Status providers — called periodically to render a check mark next to
    // the relevant step. Each should return true once the user has completed
    // that step.
    void setAudioConfiguredProvider(StatusProvider p)   { isAudioConfigured = std::move(p); }
    void setPluginsScannedProvider(StatusProvider p)    { isPluginsScanned = std::move(p); }
    void setInstrumentTrackProvider(StatusProvider p)   { hasInstrumentTrack = std::move(p); }
    void setExistingTrackOpenedProvider(StatusProvider p) { hasOpenedExistingTrack = std::move(p); }

    // Persisted setting — true if the user wants the Quickstart tab to open
    // automatically on app start.
    static bool getShowOnStartup();
    static void setShowOnStartup(bool show);

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void buttonClicked(juce::Button*) override;

    // ---- Step card ----------------------------------------------------------
    struct StepCard
    {
        int number = 0;
        juce::String title;
        juce::String description;
        juce::String actionLabel;
        juce::Rectangle<int> bounds;
        std::unique_ptr<juce::TextButton> button;
        bool done = false;
    };

    void layoutSteps();
    void paintHeader(juce::Graphics&, juce::Rectangle<int> area);
    void paintStep(juce::Graphics&, const StepCard& step);

    std::array<StepCard, 4> steps;
    std::unique_ptr<juce::ToggleButton> showOnStartupToggle;

    ActionCallback onConfigureAudio;
    ActionCallback onScanPlugins;
    ActionCallback onCreateInstrumentTrack;
    ActionCallback onOpenExistingTrack;

    StatusProvider isAudioConfigured;
    StatusProvider isPluginsScanned;
    StatusProvider hasInstrumentTrack;
    StatusProvider hasOpenedExistingTrack;

    juce::Rectangle<int> headerBounds;
    juce::Rectangle<int> contentBounds;
    juce::Rectangle<int> footerBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QuickstartComponent)
};
