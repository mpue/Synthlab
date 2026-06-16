/*
  ==============================================================================

    QuickstartComponent.h
    Welcome / onboarding screen — opens as a tab in the editor area and walks
    the user through the three first steps required to get going.

    The visible content lives inside a Viewport so the page stays fully
    readable (and scrollable) on small screens instead of getting clipped.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include <functional>
#include <memory>

class QuickstartComponent : public juce::Component
{
public:
    using ActionCallback = std::function<void()>;
    using StatusProvider = std::function<bool()>;
    using PathCallback   = std::function<void(const juce::String&)>;
    using RecentProvider = std::function<juce::StringArray()>;

    QuickstartComponent();
    ~QuickstartComponent() override;

    void setOnConfigureAudio(ActionCallback cb);
    void setOnScanPlugins(ActionCallback cb);
    void setOnCreateInstrumentTrack(ActionCallback cb);
    void setOnOpenExistingTrack(ActionCallback cb);
    void setOnOpenRecentFile(PathCallback cb);
    void setRecentFilesProvider(RecentProvider p);

    // Status providers — called periodically to render a check mark next to
    // the relevant step. Each should return true once the user has completed
    // that step.
    void setAudioConfiguredProvider(StatusProvider p);
    void setPluginsScannedProvider(StatusProvider p);
    void setInstrumentTrackProvider(StatusProvider p);
    void setExistingTrackOpenedProvider(StatusProvider p);

    // Persisted setting — true if the user wants the Quickstart tab to open
    // automatically on app start.
    static bool getShowOnStartup();
    static void setShowOnStartup(bool show);

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    // Holds all the actual layout/painting; lives inside the viewport so it
    // can grow taller than the visible area and be scrolled.
    class Content;

    juce::Viewport             viewport;
    std::unique_ptr<Content>   content;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QuickstartComponent)
};
