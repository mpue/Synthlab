#include "SessionView.h"
#include "SessionGrid.h"
#include "SessionTrackNavigator.h"
#include "../Project/Project.h"
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

    bool isAcceptedAudioFile (const juce::File& file)
    {
        const auto ext = file.getFileExtension().toLowerCase();
        return ext == ".wav"  || ext == ".mp3" || ext == ".flac"
            || ext == ".ogg"  || ext == ".aiff" || ext == ".aif"
            || ext == ".m4a"  || ext == ".wma";
    }

    long nextFreeSampleOffset (Track* t)
    {
        long maxEnd = 0;
        for (int i = 0; i < t->getNumRegions(); ++i)
        {
            Region* r = t->getRegionByIndex(i);
            if (r == nullptr) continue;
            const long end = r->getSampleOffset() + (long) r->getNumSamples();
            if (end > maxEnd) maxEnd = end;
        }
        return maxEnd;
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
                          TransportSampleProvider provider,
                          SessionModeCallback     setMode)
    : grid (g), quantizer (q),
      sampleProvider (std::move (provider)),
      setSessionModeCallback (std::move (setMode))
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

    sessionModeToggle.setClickingTogglesState (true);
    sessionModeToggle.setColour (juce::TextButton::buttonOnColourId,
                                 juce::Colour (0xff2f8f3d));
    sessionModeToggle.onClick = [this]
    {
        sessionModeOn = sessionModeToggle.getToggleState();
        sessionModeToggle.setButtonText (sessionModeOn ? "Session: ON"
                                                       : "Session: OFF");
        if (setSessionModeCallback)
            setSessionModeCallback (sessionModeOn);
    };
    addAndMakeVisible (sessionModeToggle);

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

void SessionView::setSessionModeIndicator (bool on)
{
    sessionModeOn = on;
    sessionModeToggle.setToggleState (on, juce::dontSendNotification);
    sessionModeToggle.setButtonText (on ? "Session: ON" : "Session: OFF");
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
    quantBox         .setBounds (8,   3, 110, toolbarHeight - 6);
    stopAllButton    .setBounds (126, 3,  80, toolbarHeight - 6);
    sessionModeToggle.setBounds (214, 3, 110, toolbarHeight - 6);
    sceneCountLabel  .setBounds (getWidth() - 160, 3, 152, toolbarHeight - 6);
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

    // Drag-drop target highlight. Drawn underneath the slot so the slot
    // border still shows through. Scene index can exceed nS (the row that
    // will be created by the drop) -- in that case the highlight appears
    // below the current bottom row, signalling a new scene will appear.
    if (dropTargetTrack >= 0 && dropTargetScene >= 0
        && dropTargetTrack < nT)
    {
        auto hl = slotBounds (dropTargetTrack, dropTargetScene);
        g.setColour (juce::Colour (0xff5da9ff).withAlpha (0.35f));
        g.fillRect (hl);
        g.setColour (juce::Colour (0xff5da9ff));
        g.drawRect (hl, 2);
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

    const auto hit = hitTest (e.getPosition());

    // Right-click on an empty MIDI/INSTRUMENT slot opens the "Record here"
    // context menu. Right-click elsewhere is currently a no-op (future
    // home of per-slot quantisation overrides, etc.).
    if (e.mods.isPopupMenu())
    {
        if (hit.kind == HitKind::Slot
            && grid->getRegionAt (hit.trackIndex, hit.sceneIndex) == nullptr)
        {
            Track* t = grid->getTrack (hit.trackIndex);
            if (t != nullptr
                && (t->getType() == Track::Type::MIDI
                 || t->getType() == Track::Type::INSTRUMENT))
            {
                showRecordContextMenu (hit.trackIndex);
            }
        }
        return;
    }

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

void SessionView::showRecordContextMenu (int trackIndex)
{
    juce::PopupMenu sub;
    sub.addItem (1, "1 bar");
    sub.addItem (2, "2 bars");
    sub.addItem (4, "4 bars");
    sub.addItem (8, "8 bars");

    juce::PopupMenu menu;
    menu.addSubMenu ("Record here", sub);

    juce::Component::SafePointer<SessionView> self (this);
    menu.showMenuAsync (juce::PopupMenu::Options{},
        [self, trackIndex] (int result)
        {
            if (self == nullptr)              return;
            if (result < 1 || result > 8)     return;
            self->recordIntoNewClip (trackIndex, result);
        });
}

void SessionView::recordIntoNewClip (int trackIndex, int numBars)
{
    if (grid == nullptr || quantizer == nullptr) return;

    Track* t = grid->getTrack (trackIndex);
    if (t == nullptr) return;
    SessionTrackNavigator* nav = grid->getNavigator (trackIndex);
    if (nav == nullptr) return;

    const double sr  = Project::getInstance()->getSampleRate();
    const float  bpm = Project::getInstance()->getTempo();
    if (sr <= 0.0 || bpm <= 0.0f) return;

    // 4/4 assumed -- matches LaunchQuantizer's defaults. Time-sig plumbing
    // would let us honour exotic meters here later.
    const long samplesPerQuarter = (long) (sr * 60.0 / (double) bpm);
    const long regionLength      = samplesPerQuarter * 4 * (long) numBars;
    const long offset            = nextFreeSampleOffset (t);

    MidiRegion* r = t->addMidiRegion (sr, offset, regionLength);
    if (r == nullptr) return;

    // Arm the track for recording. Visual state of the arrangement-view R
    // button won't auto-update -- it reads track->isRecording() at
    // construction, not via a listener -- but TrackIn::eventReceived sees
    // the flag and writes incoming MIDI into the active session region.
    t->setRecording (true);

    const int newIndex = t->getNumRegions() - 1;
    quantizer->schedule (nav, newIndex, quantization, currentSample());
    grid->requestRebuild();
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

bool SessionView::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (const auto& path : files)
        if (isAcceptedAudioFile (juce::File (path)))
            return true;
    return false;
}

void SessionView::fileDragEnter (const juce::StringArray& files, int x, int y)
{
    fileDragMove (files, x, y);
}

void SessionView::fileDragMove (const juce::StringArray& /*files*/, int x, int y)
{
    int newTrack = -1;
    int newScene = -1;

    if (grid != nullptr)
    {
        const auto hit = hitTest ({ x, y });
        const bool overTrackColumn = (hit.kind == HitKind::Slot
                                   || hit.kind == HitKind::TrackHeader)
                                   && hit.trackIndex >= 0;
        if (overTrackColumn)
        {
            Track* t = grid->getTrack (hit.trackIndex);
            if (t != nullptr)
            {
                newTrack = hit.trackIndex;
                newScene = t->getNumRegions(); // next free scene index
            }
        }
    }

    if (newTrack != dropTargetTrack || newScene != dropTargetScene)
    {
        dropTargetTrack = newTrack;
        dropTargetScene = newScene;
        repaint();
    }
}

void SessionView::fileDragExit (const juce::StringArray& /*files*/)
{
    if (dropTargetTrack >= 0 || dropTargetScene >= 0)
    {
        dropTargetTrack = -1;
        dropTargetScene = -1;
        repaint();
    }
}

void SessionView::filesDropped (const juce::StringArray& files, int x, int y)
{
    // Resolve target from the final cursor position rather than relying on
    // the dragMove cache -- the user may have moved the cursor and we want
    // the drop to land where they released.
    Hit hit{};
    if (grid != nullptr)
        hit = hitTest ({ x, y });

    Track* target = nullptr;
    if ((hit.kind == HitKind::Slot || hit.kind == HitKind::TrackHeader)
        && hit.trackIndex >= 0)
        target = grid->getTrack (hit.trackIndex);

    dropTargetTrack = -1;
    dropTargetScene = -1;

    if (target == nullptr)
    {
        repaint();
        return;
    }

    const double sr = Project::getInstance()->getSampleRate();
    long offset = nextFreeSampleOffset (target);
    bool addedAny = false;

    for (const auto& path : files)
    {
        juce::File file (path);
        if (! file.existsAsFile())            continue;
        if (! isAcceptedAudioFile (file))     continue;

        Region* r = target->addRegion (file.getFileNameWithoutExtension(),
                                       file, sr, offset);
        if (r != nullptr)
        {
            offset += (long) r->getNumSamples();
            Project::getInstance()->addAudioFile (file.getFileNameWithoutExtension(),
                                                  file.getFullPathName());
            addedAny = true;
        }
    }

    // Track::addRegion doesn't broadcast via TrackNavigator, so trigger the
    // grid rebuild manually -- otherwise the new region would only appear
    // on the next unrelated topology change.
    if (addedAny)
        grid->requestRebuild();

    repaint();
}

} // namespace synthlab::session
