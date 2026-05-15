// Clip-launcher grid UI over a SessionGrid. Ableton-style layout:
// columns are tracks, rows are scenes, with a toolbar (global quantisation
// + master stop) along the top.
//
// Slot state is derived from the navigator's atomic active/pending indices
// every timer tick, so the audio thread is never touched from here. Clicks
// on cells/scene-headers translate into LaunchQuantizer schedules through
// the grid. A std::function<long()> supplies the current transport sample
// at click time -- the caller wires this to the engine's transport.
//
// Caller owns the grid, quantizer and navigators. SessionView is a
// standalone Component; embed it wherever (MainTabbedComponent tab,
// dedicated window, juce::Viewport for scrolling) -- it does not assume
// a particular host.

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "LaunchQuantizer.h"
#include <functional>

namespace synthlab::session {

class SessionGrid;

class SessionView : public juce::Component,
                    private juce::Timer
{
public:
    using TransportSampleProvider = std::function<long()>;

    SessionView (SessionGrid*            grid,
                 LaunchQuantizer*        quantizer,
                 TransportSampleProvider transportSampleProvider);
    ~SessionView() override;

    void setQuantization (Quantization q);
    Quantization getQuantization() const noexcept { return quantization; }

    // Cell sizing — caller can tune to taste before adding to the hierarchy.
    void setCellSize (int width, int height);

    // Natural unconstrained size for the current grid contents. Useful when
    // wrapping in juce::Viewport: viewport.setViewedComponent(view); then
    // view->setSize(view->getPreferredWidth(), view->getPreferredHeight()).
    int getPreferredWidth()  const noexcept;
    int getPreferredHeight() const noexcept;

    void paint   (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    void timerCallback() override;

    enum class HitKind { None, TrackHeader, SceneHeader, Slot };
    struct Hit
    {
        HitKind kind        = HitKind::None;
        int     trackIndex  = -1;
        int     sceneIndex  = -1;
    };
    Hit hitTest (juce::Point<int> p) const;

    juce::Rectangle<int> slotBounds        (int t, int s) const;
    juce::Rectangle<int> trackHeaderBounds (int t)        const;
    juce::Rectangle<int> sceneHeaderBounds (int s)        const;
    juce::Rectangle<int> gridArea() const;

    long currentSample() const;

    SessionGrid*            grid          = nullptr;
    LaunchQuantizer*        quantizer     = nullptr;
    TransportSampleProvider sampleProvider;
    Quantization            quantization  = Quantization::OneBar;

    int toolbarHeight     = 30;
    int trackHeaderHeight = 22;
    int sceneHeaderWidth  = 90;
    int cellWidth         = 110;
    int cellHeight        = 26;

    int blinkPhase = 0;          // counts timer ticks; queued slots blink

    juce::ComboBox   quantBox;
    juce::TextButton stopAllButton { "Stop All" };
    juce::Label      sceneCountLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SessionView)
};

} // namespace synthlab::session
