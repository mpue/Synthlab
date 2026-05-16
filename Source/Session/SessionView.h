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
                    public  juce::FileDragAndDropTarget,
                    private juce::Timer
{
public:
    using TransportSampleProvider = std::function<long()>;
    using SessionModeCallback     = std::function<void(bool)>;

    SessionView (SessionGrid*            grid,
                 LaunchQuantizer*        quantizer,
                 TransportSampleProvider transportSampleProvider,
                 SessionModeCallback     setSessionModeCallback);
    ~SessionView() override;

    void setQuantization (Quantization q);
    Quantization getQuantization() const noexcept { return quantization; }

    // Reflects the current "session mode on / off" state in the toolbar
    // button without invoking the callback. Useful when something else
    // (load project, undo, etc.) flips the mode.
    void setSessionModeIndicator (bool on);

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

    // FileDragAndDropTarget -- accept audio files dropped onto a track
    // column. The drop creates a new region on that track; SessionGrid
    // rebuilds afterwards so the clip appears in the next free scene row.
    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void fileDragEnter          (const juce::StringArray& files, int x, int y) override;
    void fileDragMove           (const juce::StringArray& files, int x, int y) override;
    void fileDragExit           (const juce::StringArray& files) override;
    void filesDropped           (const juce::StringArray& files, int x, int y) override;

private:
    void timerCallback() override;
    void showRecordContextMenu (int trackIndex);
    void recordIntoNewClip     (int trackIndex, int numBars);

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
    SessionModeCallback     setSessionModeCallback;
    Quantization            quantization  = Quantization::OneBar;
    bool                    sessionModeOn = false;

    int toolbarHeight     = 30;
    int trackHeaderHeight = 22;
    int sceneHeaderWidth  = 90;
    int cellWidth         = 110;
    int cellHeight        = 26;

    int blinkPhase = 0;          // counts timer ticks; queued slots blink

    // During a file-drag: which track column the drop will land on and the
    // scene index the new region will become (= track->getNumRegions()).
    // -1 / -1 when no drag is in progress or the drag is over an
    // unacceptable target (toolbar, outside grid, etc.).
    int dropTargetTrack = -1;
    int dropTargetScene = -1;

    juce::ComboBox   quantBox;
    juce::TextButton stopAllButton    { "Stop All" };
    juce::TextButton sessionModeToggle { "Session: OFF" };
    juce::Label      sceneCountLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SessionView)
};

} // namespace synthlab::session
