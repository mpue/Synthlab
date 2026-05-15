#include "SessionView.h"
#include "SessionGrid.h"
#include "SessionTrackNavigator.h"
#include "../Tracks/Region.h"
#include "../Tracks/Track.h"

namespace synthlab::session {

namespace
{
    constexpr int kBlinkPeriodTicks = 8; // ~266 ms at 30 Hz

    juce::Colour colourForState (SlotState s, bool blinkOn)
    {
        switch (s)
        {
            case SlotState::Empty:     return juce::Colour (0xff1c1c20);
            case SlotState::HasRegion: return juce::Colour (0xff3b3b46);
            case SlotState::Queued:    return blinkOn ? juce::Colour (0xffd9a527)
                                                       : juce::Colour (0xff3b3b46);
            case SlotState::Playing:   return juce::Colour (0xff2f8f3d);
            case SlotState::Recording: return juce::Colour (0xffa12a2a);
        }
        return juce::Colours::darkgrey;
    }

    Quantization quantizationForComboId (int id)
    {
        switch (id)
        {
            case 1: return Quantization::None;
            case 2: return Quantization::OneSixteenth;
            case 3: return Quantization::OneEighth;
            case 4: return Quantization::OneQuarter;
            case 5: return Quantization::OneBar;
            case 6: return Quantization::TwoBars;
            case 7: return Quantization::FourBars;
            case 8: return Quantization::EightBars;
        }
        return Quantization::OneBar;
    }

    int comboIdForQuantization (Quantization q)
    {
        switch (q)
        {
            case Quantization::None:         return 1;
            case Quantization::OneSixteenth: return 2;
            case Quantization::OneEighth:    return 3;
            case Quantization::OneQuarter:   return 4;
            case Quantization::OneBar:       return 5;
            case Quantization::TwoBars:      return 6;
            case Quantization::FourBars:     return 7;
            case Quantization::EightBars:    return 8;
        }
        return 5;
    }
}

SessionView::SessionView (SessionGrid*            g,
                          LaunchQuantizer*        q,
                          TransportSampleProvider provider)
    : grid (g), quantizer (q), sampleProvider (std::move (provider))
{
    quantBox.addItem ("None",   1);
    quantBox.addItem ("1/16",   2);
    quantBox.addItem ("1/8",    3);
    quantBox.addItem ("1/4",    4);
    quantBox.addItem ("1 Bar",  5);
    quantBox.addItem ("2 Bars", 6);
    quantBox.addItem ("4 Bars", 7);
    quantBox.addItem ("8 Bars", 8);
    quantBox.setSelectedId (comboIdForQuantization (quantization),
                            juce::dontSendNotification);
    quantBox.onChange = [this]
    {
        quantization = quantizationForComboId (quantBox.getSelectedId());
    };
    addAndMakeVisible (quantBox);

    stopAllButton.onClick = [this]
    {
        if (grid != nullptr)
            grid->stopAll (quantization, currentSample());
    };
    addAndMakeVisible (stopAllButton);

    sceneCountLabel.setJustificationType (juce::Justification::centredRight);
    sceneCountLabel.setColour (juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible (sceneCountLabel);

    startTimerHz (30);
}

SessionView::~SessionView() { stopTimer(); }

void SessionView::setQuantization (Quantization q)
{
    quantization = q;
    quantBox.setSelectedId (comboIdForQuantization (q), juce::dontSendNotification);
}

void SessionView::setCellSize (int w, int h)
{
    cellWidth  = juce::jmax (40, w);
    cellHeight = juce::jmax (16, h);
    resized();
    repaint();
}

int SessionView::getPreferredWidth() const noexcept
{
    const int n = grid != nullptr ? grid->getNumTracks() : 0;
    return sceneHeaderWidth + n * cellWidth;
}

int SessionView::getPreferredHeight() const noexcept
{
    const int n = grid != nullptr ? grid->getNumScenes() : 0;
    return toolbarHeight + trackHeaderHeight + n * cellHeight;
}

void SessionView::resized()
{
    quantBox       .setBounds (8,   3, 110, toolbarHeight - 6);
    stopAllButton  .setBounds (126, 3,  80, toolbarHeight - 6);
    sceneCountLabel.setBounds (getWidth() - 160, 3, 152, toolbarHeight - 6);
}

juce::Rectangle<int> SessionView::gridArea() const
{
    return { 0, toolbarHeight,
             getWidth(), juce::jmax (0, getHeight() - toolbarHeight) };
}

juce::Rectangle<int> SessionView::slotBounds (int t, int s) const
{
    const int x = sceneHeaderWidth + t * cellWidth;
    const int y = toolbarHeight + trackHeaderHeight + s * cellHeight;
    return { x, y, cellWidth, cellHeight };
}

juce::Rectangle<int> SessionView::trackHeaderBounds (int t) const
{
    const int x = sceneHeaderWidth + t * cellWidth;
    return { x, toolbarHeight, cellWidth, trackHeaderHeight };
}

juce::Rectangle<int> SessionView::sceneHeaderBounds (int s) const
{
    const int y = toolbarHeight + trackHeaderHeight + s * cellHeight;
    return { 0, y, sceneHeaderWidth, cellHeight };
}

void SessionView::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff141416));

    // Toolbar separator.
    g.setColour (juce::Colour (0xff2a2a30));
    g.fillRect (0, toolbarHeight - 1, getWidth(), 1);

    if (grid == nullptr) return;

    const int nT = grid->getNumTracks();
    const int nS = grid->getNumScenes();

    sceneCountLabel.setText (juce::String (nT) + " tracks  "
                             + juce::String (nS) + " scenes",
                             juce::dontSendNotification);

    const bool blinkOn = (blinkPhase < kBlinkPeriodTicks / 2);

    // Track headers.
    g.setFont (12.0f);
    for (int t = 0; t < nT; ++t)
    {
        auto bounds = trackHeaderBounds (t);
        g.setColour (juce::Colour (0xff222228));
        g.fillRect (bounds);
        g.setColour (juce::Colour (0xff3a3a44));
        g.drawRect (bounds);
        g.setColour (juce::Colours::white);

        juce::String label;
        if (auto* track = grid->getTrack (t))
            label = track->getName();
        if (label.isEmpty()) label = "Track " + juce::String (t + 1);

        g.drawText (label, bounds.reduced (6, 0),
                    juce::Justification::centredLeft, true);
    }

    // Scene headers.
    for (int s = 0; s < nS; ++s)
    {
        auto bounds = sceneHeaderBounds (s);
        g.setColour (juce::Colour (0xff262630));
        g.fillRect (bounds);
        g.setColour (juce::Colour (0xff3a3a44));
        g.drawRect (bounds);
        g.setColour (juce::Colours::lightgrey);
        g.drawText ("Scene " + juce::String (s + 1),
                    bounds.reduced (8, 0),
                    juce::Justification::centredLeft, true);
    }

    // Slots.
    for (int t = 0; t < nT; ++t)
    {
        for (int s = 0; s < nS; ++s)
        {
            const auto state  = grid->getSlotState (t, s);
            const auto bounds = slotBounds (t, s);

            g.setColour (colourForState (state, blinkOn));
            g.fillRect (bounds);
            g.setColour (juce::Colour (0xff10101a));
            g.drawRect (bounds);

            if (state != SlotState::Empty)
            {
                juce::String name;
                if (auto* r = grid->getRegionAt (t, s))
                    name = r->getName();
                if (name.isEmpty()) name = "Clip " + juce::String (s + 1);

                g.setColour (state == SlotState::Playing
                                ? juce::Colours::white
                                : juce::Colours::lightgrey);
                g.drawText (name, bounds.reduced (6, 0),
                            juce::Justification::centredLeft, true);
            }
        }
    }
}

SessionView::Hit SessionView::hitTest (juce::Point<int> p) const
{
    if (p.y < toolbarHeight) return {};
    if (grid == nullptr)     return {};

    const int nT = grid->getNumTracks();
    const int nS = grid->getNumScenes();

    // Track header row.
    if (p.y < toolbarHeight + trackHeaderHeight)
    {
        if (p.x < sceneHeaderWidth) return {};
        const int t = (p.x - sceneHeaderWidth) / cellWidth;
        if (t < 0 || t >= nT) return {};
        return { HitKind::TrackHeader, t, -1 };
    }

    // Scene header column.
    if (p.x < sceneHeaderWidth)
    {
        const int s = (p.y - toolbarHeight - trackHeaderHeight) / cellHeight;
        if (s < 0 || s >= nS) return {};
        return { HitKind::SceneHeader, -1, s };
    }

    // Slot.
    const int t = (p.x - sceneHeaderWidth) / cellWidth;
    const int s = (p.y - toolbarHeight - trackHeaderHeight) / cellHeight;
    if (t < 0 || t >= nT || s < 0 || s >= nS) return {};
    return { HitKind::Slot, t, s };
}

void SessionView::mouseDown (const juce::MouseEvent& e)
{
    if (grid == nullptr) return;

    const auto hit    = hitTest (e.getPosition());
    const long sample = currentSample();

    switch (hit.kind)
    {
        case HitKind::Slot:
            grid->launchSlot (hit.trackIndex, hit.sceneIndex,
                              quantization, sample);
            break;
        case HitKind::SceneHeader:
            grid->launchScene (hit.sceneIndex, quantization, sample);
            break;
        case HitKind::TrackHeader:
            grid->stopTrack (hit.trackIndex, quantization, sample);
            break;
        case HitKind::None:
            break;
    }
    repaint();
}

void SessionView::timerCallback()
{
    blinkPhase = (blinkPhase + 1) % kBlinkPeriodTicks;
    repaint();
}

long SessionView::currentSample() const
{
    return sampleProvider ? sampleProvider() : 0L;
}

} // namespace synthlab::session
