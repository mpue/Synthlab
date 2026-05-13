#include "PianoRollComponent.h"
#include "Tracks/MidiRegion.h"
#include "Tracks/Track.h"

PianoRollComponent::PianoRollComponent(MidiRegion* region, double sampleRate)
    : midiRegion(region), sampleRate(sampleRate)
{
    if (midiRegion != nullptr)
        midiRegion->addComponentListener(this);

    setWantsKeyboardFocus(true);
    setMouseClickGrabsKeyboardFocus(true);
    setInterceptsMouseClicks(true, true);
    addKeyListener(this);

    // Initialize note colours per channel
    for (int i = 0; i < 16; ++i) {
        noteColours[i] = juce::Colour::fromHSV(i / 16.0f, 0.7f, 0.9f, 1.0f);
    }

    // Start animation timer
    startTimer(30); // 30ms for smooth animation

    // Initialize custom scale (C major as default)
    customScale = { 0, 2, 4, 5, 7, 9, 11 };

    setMouseCursor(getCursorForTool(currentTool));
}

PianoRollComponent::~PianoRollComponent()
{
    if (midiRegion != nullptr)
        midiRegion->removeComponentListener(this);
}

void PianoRollComponent::componentBeingDeleted(juce::Component& /*component*/)
{
    // The MidiRegion we're editing is being destroyed. Sever the pointer,
    // clear any state that references it, and repaint to show the empty state.
    midiRegion = nullptr;
    selectedMessages.clear();
    resizingMessage = nullptr;
    resizingPartner = nullptr;
    isDragging = false;
    isResizing = false;
    isVelocityEditing = false;
    dragPreviewNotes.clear();
    repaint();
}

void PianoRollComponent::timerCallback()
{
    // Animate pulse for selected notes and playhead
    pulseAnimation += 0.1f;
    if (pulseAnimation > juce::MathConstants<float>::twoPi)
        pulseAnimation = 0.0f;

    if (!selectedMessages.empty() || playbackPosition >= 0.0) {
        repaint();
    }
}

// =================== DRAWING METHODS ===================

void PianoRollComponent::paint(juce::Graphics& g)
{
    if (midiRegion == nullptr)
    {
        g.fillAll(juce::Colour(0xff1a1a1a));
        g.setColour(juce::Colours::grey);
        g.setFont(16.0f);
        g.drawFittedText("Region deleted", getLocalBounds(), juce::Justification::centred, 1);
        return;
    }

    // Performance optimization: clip drawing region
    auto clipBounds = g.getClipBounds();

    if (showGrid) drawGrid(g);
    drawNotes(g);
    drawPiano(g);

    if (!controllerLanes.empty()) drawControllerLanes(g);
    if (showVelocityAsHeight) drawVelocityBars(g);
    if (hasLoopRegion) drawLoopRegion(g);
    if (playbackPosition >= 0.0) drawPlayhead(g);
    if (drawSelectionRect) drawSelectionRectangle(g);
    if (snapToGrid && isDragging) drawSnapGuides(g);
}

void PianoRollComponent::drawGrid(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a1a)); // Dark background
    auto bounds = getLocalBounds();

    // Piano roll grid
    for (int p = 0; p < 128; ++p) {
        int y = getYForPitch(p);
        bool isOctave = (p % 12 == 0);
        bool isSemitone = isBlackKey(p);

        if (isOctave) {
            g.setColour(juce::Colours::white.withAlpha(0.15f));
            g.drawLine((float)pianoWidth, (float)y, (float)bounds.getWidth(), (float)y, 2.0f);
        }
        else if (!isSemitone) {
            g.setColour(juce::Colours::white.withAlpha(0.08f));
            g.drawLine((float)pianoWidth, (float)y, (float)bounds.getWidth(), (float)y, 1.0f);
        }

        // Scale highlighting
        if (!customScale.empty() && std::find(customScale.begin(), customScale.end(), p % 12) != customScale.end()) {
            g.setColour(juce::Colours::lightblue.withAlpha(0.05f));
            g.fillRect(pianoWidth, y, bounds.getWidth() - pianoWidth, (int)yZoom);
        }
    }

    // Time grid
    double quarterNoteSamples = (60.0 / bpm) * sampleRate;
    double gridSamples = quarterNoteSamples * gridResolution;

    double startSample = getTimeAtX(0);
    double endSample = getTimeAtX(bounds.getWidth());

    int firstGrid = (int)(startSample / gridSamples);
    int lastGrid = (int)(endSample / gridSamples);

    for (int i = firstGrid; i <= lastGrid; ++i) {
        double sampleTime = i * gridSamples;
        int x = getXForTime(sampleTime);

        // Different line weights for different subdivisions
        bool isBar = (i % (int)(4.0 / gridResolution)) == 0;
        bool isBeat = (i % (int)(1.0 / gridResolution)) == 0;

        if (isBar) {
            g.setColour(juce::Colours::white.withAlpha(0.4f));
            g.drawLine((float)x, 0.0f, (float)x, (float)bounds.getHeight(), 2.0f);
        }
        else if (isBeat) {
            g.setColour(juce::Colours::white.withAlpha(0.2f));
            g.drawLine((float)x, 0.0f, (float)x, (float)bounds.getHeight(), 1.5f);
        }
        else {
            g.setColour(juce::Colours::white.withAlpha(0.1f));
            g.drawLine((float)x, 0.0f, (float)x, (float)bounds.getHeight(), 1.0f);
        }
    }
}

void PianoRollComponent::drawPiano(juce::Graphics& g)
{
    for (int i = 0; i < 128; ++i) {
        int y = getYForPitch(i);
        bool isBlack = isBlackKey(i);

        // Piano key background
        g.setColour(isBlack ? juce::Colour(0xff2a2a2a) : juce::Colour(0xfff8f8f8));
        g.fillRect(0, y, pianoWidth, (int)yZoom);

        // Key borders
        g.setColour(juce::Colour(0xff666666));
        g.drawRect(0, y, pianoWidth, (int)yZoom, 1);

        // Octave labels
        if (i % 12 == 0) {
            int octave = (i / 12) - 1;
            g.setColour(isBlack ? juce::Colours::lightgrey : juce::Colours::darkgrey);
            g.setFont(juce::Font(9.0f, juce::Font::bold));
            g.drawText("C" + juce::String(octave), 4, y, pianoWidth - 8, (int)yZoom,
                juce::Justification::centredLeft);
        }

        // Note names (optional)
        if (showNoteNames && yZoom > 16) {
            static const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F",
                                            "F#", "G", "G#", "A", "A#", "B" };
            g.setColour(isBlack ? juce::Colours::lightgrey : juce::Colours::darkgrey);
            g.setFont(8.0f);
            g.drawText(noteNames[i % 12], pianoWidth - 20, y, 18, (int)yZoom,
                juce::Justification::centred);
        }

        // Highlight playing notes
        // This would be connected to your audio engine
        // if (isNotePlaying(i)) {
        //     g.setColour(juce::Colours::red.withAlpha(0.3f));
        //     g.fillRect(0, y, pianoWidth, (int)yZoom);
        // }
    }

    // Piano separator
    g.setColour(juce::Colour(0xff444444));
    g.drawLine((float)pianoWidth, 0.0f, (float)pianoWidth, (float)getHeight(), 2.0f);
}

void PianoRollComponent::drawNotes(juce::Graphics& g)
{
    if (midiRegion == nullptr) {
        return;
    }

    auto& messages = midiRegion->getMidiMessageList();

    // Performance: only draw notes in visible area
    auto clipBounds = g.getClipBounds();
    double startTime = getTimeAtX(clipBounds.getX());
    double endTime = getTimeAtX(clipBounds.getRight());

    if (messages.empty())
        return;

    for (auto* msg : messages) {
        if (!msg || !msg->isNoteOn()) continue;

        MidiMessage* noteOff = findNoteOffForNoteOn(msg);
        if (!noteOff) continue;

        double duration = noteOff->getTimeStamp() - msg->getTimeStamp();
        if (duration <= 0.0) continue;

        // Visibility culling
        double noteStart = msg->getTimeStamp();
        double noteEnd = noteStart + duration;
        if (noteEnd < startTime || noteStart > endTime) continue;

        bool isSelected = selectedMessages.find(msg) != selectedMessages.end();

        // Check if this note has a drag preview
        bool hasPreview = false;
        DragPreviewNote previewData;

        if (isDragging && !dragPreviewNotes.empty()) {
            for (const auto& preview : dragPreviewNotes) {
                if (preview.originalNote == msg) {
                    hasPreview = true;
                    previewData = preview;
                    break;
                }
            }
        }

        if (hasPreview) {
            // Draw preview version
            drawNotePreview(g, msg, previewData, isSelected);
        }
        else {
            // Draw normal version
            drawNote(g, *msg, duration, isSelected);
        }
    }

    // Draw resize preview if resizing
    if (isResizing && resizingMessage && dragPreviewEndTime > 0) {
        drawResizePreview(g);
    }
}


void PianoRollComponent::drawNotePreview(juce::Graphics& g, MidiMessage* originalMsg,
    const DragPreviewNote& preview, bool isSelected)
{
    // Create temporary message for preview drawing
    MidiMessage tempMsg(*originalMsg);
    tempMsg.setTimeStamp(preview.previewStartTime);
    tempMsg.setNoteNumber(preview.previewPitch);

    double previewDuration = preview.previewEndTime - preview.previewStartTime;

    // Draw with preview styling (slightly transparent)
    auto rect = getNoteRect(tempMsg, previewDuration);
    if (rect.getWidth() < 1 || rect.getHeight() < 1) return;

    juce::Colour noteColour = getColourForChannel(tempMsg.getChannel() - 1);
    noteColour = noteColour.withAlpha(0.7f); // Make preview semi-transparent

    if (isSelected) {
        float pulse = std::sin(pulseAnimation) * 0.2f + 0.8f;
        noteColour = noteColour.brighter(pulse * 0.3f);

        // Preview selection border
        g.setColour(juce::Colours::orange.withAlpha(0.6f));
        g.drawRect(rect.expanded(1), 2);
    }

    // Draw preview note
    g.setColour(noteColour);
    g.fillRoundedRectangle(rect.toFloat(), 2.0f);

    g.setColour(noteColour.darker(0.3f));
    g.drawRoundedRectangle(rect.toFloat(), 2.0f, 1.0f);
}

void PianoRollComponent::drawResizePreview(juce::Graphics& g)
{
    if (!resizingMessage) return;

    auto rect = getNoteRect(*resizingMessage, dragPreviewEndTime - resizingMessage->getTimeStamp());

    g.setColour(juce::Colours::orange.withAlpha(0.3f));
    g.fillRoundedRectangle(rect.toFloat(), 2.0f);

    g.setColour(juce::Colours::orange);
    g.drawRoundedRectangle(rect.toFloat(), 2.0f, 1.0f);
}


void PianoRollComponent::drawNote(juce::Graphics& g, const juce::MidiMessage& msg, double duration, bool isSelected)
{
    auto rect = getNoteRect(msg, duration);
    if (rect.getWidth() < 1 || rect.getHeight() < 1) return;

    int velocity = msg.getVelocity();
    int channel = msg.getChannel() - 1; // 0-based

    juce::Colour noteColour;

    if (showVelocityAsColour) {
        noteColour = getColourForVelocity(velocity);
    }
    else {
        noteColour = getColourForChannel(channel);
    }

    // Selection highlighting
    if (isSelected) {
        float pulse = std::sin(pulseAnimation) * 0.2f + 0.8f;
        noteColour = noteColour.brighter(pulse * 0.5f);

        // Selection border
        g.setColour(juce::Colours::yellow.withAlpha(0.8f));
        g.drawRect(rect.expanded(1), 2);
    }

    // Velocity-based height
    if (showVelocityAsHeight) {
        float velFactor = velocity / 127.0f;
        int velHeight = juce::jmax(2, (int)(rect.getHeight() * velFactor));
        rect = rect.withHeight(velHeight).withBottom(rect.getBottom());
    }

    // Main note body
    g.setColour(noteColour);
    g.fillRoundedRectangle(rect.toFloat(), 2.0f);

    // Note border
    g.setColour(noteColour.darker(0.3f));
    g.drawRoundedRectangle(rect.toFloat(), 2.0f, 1.0f);

    // Velocity indicator (small bar at left)
    if (rect.getWidth() > 8) {
        float velFactor = velocity / 127.0f;
        g.setColour(juce::Colours::white.withAlpha(velFactor * 0.6f));
        g.fillRect(rect.getX() + 1, rect.getY() + 1, 2, rect.getHeight() - 2);
    }

    // Note name text (if large enough)
    if (rect.getWidth() > 30 && rect.getHeight() > 12) {
        static const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F",
                                        "F#", "G", "G#", "A", "A#", "B" };
        int noteNum = msg.getNoteNumber();
        int octave = (noteNum / 12) - 1;
        juce::String noteName = juce::String(noteNames[noteNum % 12]) + juce::String(octave);

        g.setColour(noteColour.contrasting(0.8f));
        g.setFont(juce::Font(8.0f));
        g.drawText(noteName, rect.reduced(4), juce::Justification::centredLeft);

        // Velocity text
        if (rect.getWidth() > 50) {
            g.drawText(juce::String(velocity), rect.reduced(4), juce::Justification::centredRight);
        }
    }
}

void PianoRollComponent::drawVelocityBars(juce::Graphics& g)
{
    auto& messages = midiRegion->getMidiMessageList();

    for (auto* msg : messages) {
        if (!msg || !msg->isNoteOn()) continue;

        MidiMessage* noteOff = findNoteOffForNoteOn(msg);
        if (!noteOff) continue;

        double duration = noteOff->getTimeStamp() - msg->getTimeStamp();
        auto velRect = getVelocityRect(*msg, duration);

        int velocity = msg->getVelocity();
        float velFactor = velocity / 127.0f;

        g.setColour(getColourForVelocity(velocity).withAlpha(0.7f));
        g.fillRect(velRect.toFloat());

        g.setColour(juce::Colours::black.withAlpha(0.3f));
        g.drawRect(velRect.toFloat(), 1.0f);
    }
}

void PianoRollComponent::drawControllerLanes(juce::Graphics& g)
{
    int yPos = getHeight() - controllerLaneHeight * (int)controllerLanes.size();

    for (auto& [ccNum, lane] : controllerLanes) {
        if (!lane.visible) continue;

        juce::Rectangle<int> laneRect(pianoWidth, yPos, getWidth() - pianoWidth, controllerLaneHeight);

        // Lane background
        g.setColour(juce::Colour(0xff2a2a2a));
        g.fillRect(laneRect);

        // Lane border
        g.setColour(juce::Colour(0xff444444));
        g.drawRect(laneRect);

        // Lane label
        g.setColour(juce::Colours::lightgrey);
        g.setFont(10.0f);
        g.drawText(lane.name, laneRect.removeFromLeft(60), juce::Justification::centred);

        // Draw automation points
        g.setColour(lane.colour);
        juce::Path automationPath;

        for (size_t i = 0; i < lane.points.size(); ++i) {
            auto [time, value] = lane.points[i];
            int x = getXForTime(time);
            int y = laneRect.getBottom() - (value / 127.0f) * laneRect.getHeight();

            if (i == 0) automationPath.startNewSubPath((float)x, (float)y);
            else automationPath.lineTo((float)x, (float)y);

            // Draw point handle
            g.fillEllipse((float)x - 3, (float)y - 3, 6, 6);
        }

        g.strokePath(automationPath, juce::PathStrokeType(2.0f));
        yPos += controllerLaneHeight;
    }
}

void PianoRollComponent::drawPlayhead(juce::Graphics& g)
{
    if (playbackPosition < 0.0) return;

    int x = getXForTime(playbackPosition);
    auto bounds = getLocalBounds();

    // Animated playhead
    float pulse = std::sin(pulseAnimation * 2.0f) * 0.3f + 0.7f;
    g.setColour(playheadColour.withAlpha(pulse));
    g.drawLine((float)x, 0.0f, (float)x, (float)bounds.getHeight(), 3.0f);

    // Playhead top indicator
    juce::Path triangle;
    triangle.addTriangle((float)x - 6, 0, (float)x + 6, 0, (float)x, 12);
    g.fillPath(triangle);
}

void PianoRollComponent::drawLoopRegion(juce::Graphics& g)
{
    if (!hasLoopRegion) return;

    int startX = getXForTime(loopStart);
    int endX = getXForTime(loopEnd);
    auto bounds = getLocalBounds();

    g.setColour(juce::Colours::green.withAlpha(0.2f));
    g.fillRect(startX, 0, endX - startX, bounds.getHeight());

    g.setColour(juce::Colours::green);
    g.drawLine((float)startX, 0.0f, (float)startX, (float)bounds.getHeight(), 2.0f);
    g.drawLine((float)endX, 0.0f, (float)endX, (float)bounds.getHeight(), 2.0f);
}

void PianoRollComponent::drawSelectionRectangle(juce::Graphics& g)
{
    g.setColour(juce::Colours::white.withAlpha(0.3f));
    g.fillRect(selectionRectangle);

    g.setColour(juce::Colours::white);
    g.drawRect(selectionRectangle, 1);
}

void PianoRollComponent::drawSnapGuides(juce::Graphics& g)
{
    if (!snapToGrid || !isDragging) return;

    // Draw snap guides for time and pitch
    double snapTime = snapTimeToGrid(getTimeAtX(lastMousePos.x));
    int snapPitch = snapPitchToScale(getPitchAtY(lastMousePos.y));

    int snapX = getXForTime(snapTime);
    int snapY = getYForPitch(snapPitch);

    g.setColour(juce::Colours::orange.withAlpha(0.8f));
    g.drawLine((float)snapX, 0.0f, (float)snapX, (float)getHeight(), 1.0f);
    g.drawLine((float)pianoWidth, (float)snapY, (float)getWidth(), (float)snapY, 1.0f);
}

// =================== TOOL HANDLING ===================

void PianoRollComponent::mouseDown(const juce::MouseEvent& e)
{
    lastMousePos = e.getPosition();
    grabKeyboardFocus();

    // Handle different tools
    switch (currentTool) {
    case Tool::SELECT:   handleSelectTool(e); break;
    case Tool::PENCIL:   handlePencilTool(e); break;
    case Tool::ERASER:   handleEraserTool(e); break;
    case Tool::VELOCITY: handleVelocityTool(e); break;
    case Tool::SPLIT:    handleSplitTool(e); break;
    default: handleSelectTool(e); break;
    }
}

void PianoRollComponent::handleSelectTool(const juce::MouseEvent& e)
{
    if (e.mods.isMiddleButtonDown()) {
        isPanning = true;
        panStart = e.getPosition();
        return;
    }

    dragStart = e.getPosition();
    isDragging = false;
    isResizing = false;
    resizingMessage = nullptr;
    isVelocityEditing = false;

    // IMPORTANT: Clear previous drag data
    originalNotePositions.clear();
    dragPreviewNotes.clear();

    MidiMessage* hitNote = getNoteAt(e.getPosition());

    if (hitNote) {
        double duration = getNoteDuration(hitNote);
        auto rect = getNoteRect(*hitNote, duration);

        bool isOverResizeHandle = (e.x >= rect.getRight() - 8) && (e.x <= rect.getRight() + 4);

        if (isOverResizeHandle) {
            isResizing = true;
            resizingMessage = hitNote;
            resizingPartner = findNoteOffForNoteOn(hitNote);
            originalDuration = duration;
            setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
        }
        else if (isOverVelocityHandle(e.getPosition(), hitNote)) {
            isVelocityEditing = true;
            resizingMessage = hitNote;
            originalVelocities[hitNote] = hitNote->getVelocity();
            setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
        }
        else {
            // Regular selection
            if (!e.mods.isCommandDown() && selectedMessages.find(hitNote) == selectedMessages.end()) {
                selectedMessages.clear();
            }

            if (e.mods.isCommandDown()) {
                if (selectedMessages.find(hitNote) != selectedMessages.end()) {
                    selectedMessages.erase(hitNote);
                }
                else {
                    selectedMessages.insert(hitNote);
                }
            }
            else {
                selectedMessages.insert(hitNote);
            }

            setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        }
    }
    else {
        // Set track marker position when clicking in the note area
        if (e.x > pianoWidth && onMarkerPositionChanged)
            onMarkerPositionChanged(getTimeAtX(e.x));

        // Start box selection or clear selection
        if (!e.mods.isCommandDown()) {
            selectedMessages.clear();
        }

        isBoxSelecting = true;
        selectionRectangle = juce::Rectangle<int>(e.getPosition(), e.getPosition());
        drawSelectionRect = true;
        setMouseCursor(juce::MouseCursor::CrosshairCursor);
    }

    notifySelectionChanged();
    repaint();
}

void PianoRollComponent::handlePencilTool(const juce::MouseEvent& e)
{
    if (e.getPosition().x <= pianoWidth) return;

    // Rechte Maustaste: Note unter Cursor loeschen — Standardverhalten in
    // jedem DAW-Pencil-Modus, ohne Toolwechsel zum Eraser.
    if (e.mods.isRightButtonDown()) {
        if (auto* hit = getNoteAt(e.getPosition())) {
            selectedMessages.erase(hit);
            deleteNotePair(hit);
            notifySelectionChanged();
            notifyRegionModified();
            repaint();
        }
        return;
    }

    // Linke Maustaste: neue Note an Klickposition mit minimaler Anfangs-
    // laenge anlegen; mouseDrag verlaengert sie, mouseUp finalisiert.
    int pitch = snapPitchToScale(getPitchAtY(e.y));
    double start    = snapToGrid ? snapTimeToGrid(getTimeAtX(e.x)) : getTimeAtX(e.x);
    double duration = getGridTimeStep();
    int velocity = velocitySensitive
        ? juce::jlimit(1, 127, (int)(127 * (1.0f - e.y / (float)getHeight())))
        : baseVelocity;

    if (auto* on = addNotePair(pitch, start, duration, velocity)) {
        pencilNoteOn    = on;
        pencilNoteOff   = findNoteOffForNoteOn(on);
        // addNotePair snappt startTime auf das Grid — den geclippten Wert
        // direkt vom NoteOn lesen, sonst verschiebt sich der Drag-Anker.
        pencilStartTime = on->getTimeStamp();
        isPencilDrawing = (pencilNoteOff != nullptr);
    }

    notifyRegionModified();
    repaint();
}

void PianoRollComponent::handleEraserTool(const juce::MouseEvent& e)
{
    MidiMessage* hitNote = getNoteAt(e.getPosition());
    if (hitNote) {
        selectedMessages.erase(hitNote);
        deleteNotePair(hitNote);
        notifySelectionChanged();
        notifyRegionModified();
        repaint();
    }
}

void PianoRollComponent::handleVelocityTool(const juce::MouseEvent& e)
{
    MidiMessage* hitNote = getNoteAt(e.getPosition());
    if (hitNote) {
        isVelocityEditing = true;
        resizingMessage = hitNote;
        originalVelocities[hitNote] = hitNote->getVelocity();

        // Calculate velocity from mouse Y position
        auto rect = getNoteRect(*hitNote, getNoteDuration(hitNote));
        float velFactor = 1.0f - (e.y - rect.getY()) / (float)rect.getHeight();
        int newVel = juce::jlimit(1, 127, (int)(velFactor * 127));

        setNoteVelocity(hitNote, newVel);
        repaint();
    }
}

void PianoRollComponent::handleSplitTool(const juce::MouseEvent& e)
{
    MidiMessage* hitNote = getNoteAt(e.getPosition());
    if (hitNote) {
        double splitTime = snapToGrid ? snapTimeToGrid(getTimeAtX(e.x)) : getTimeAtX(e.x);
        splitNote(hitNote, splitTime);
        notifyRegionModified();
        repaint();
    }
}

void PianoRollComponent::mouseDrag(const juce::MouseEvent& e)
{
    lastMousePos = e.getPosition();

    if (isPanning) {
        auto delta = e.getPosition() - panStart;
        panOffset += delta.toFloat();
        panOffset.x = juce::jmin(panOffset.x, 0.0f);
        panStart = e.getPosition();
        repaint();
        return;
    }

    // Pencil: in-progress note bis zur aktuellen Mausposition verlaengern.
    // Untere Grenze = ein Grid-Step (sonst kann die Note unhoerbar kurz
    // werden, wenn man nach links zieht).
    if (currentTool == Tool::PENCIL && isPencilDrawing && pencilNoteOff != nullptr) {
        double t = snapToGrid ? snapTimeToGrid(getTimeAtX(e.x)) : getTimeAtX(e.x);
        const double minLen = getGridTimeStep();
        const double newEnd = juce::jmax(pencilStartTime + minLen, t);
        if (newEnd != pencilNoteOff->getTimeStamp()) {
            pencilNoteOff->setTimeStamp(newEnd);
            if (midiRegion != nullptr)
                midiRegion->rebuildThumbnail();
            notifyRegionModified();
            repaint();
        }
        return;
    }

    if (isVelocityEditing && resizingMessage) {
        auto rect = getNoteRect(*resizingMessage, getNoteDuration(resizingMessage));
        float velFactor = 1.0f - (e.y - rect.getY()) / (float)rect.getHeight();
        int newVel = juce::jlimit(1, 127, (int)(velFactor * 127));
        setNoteVelocity(resizingMessage, newVel);
        repaint();
        return;
    }

    if (isBoxSelecting) {
        selectionRectangle = juce::Rectangle<int>(dragStart, e.getPosition());

        if (!e.mods.isCommandDown()) {
            selectedMessages.clear();
        }

        auto notesInRect = getNotesInRect(selectionRectangle);
        for (auto* note : notesInRect) {
            selectedMessages.insert(note);
        }
        repaint();
        return;
    }

    if (currentTool != Tool::SELECT) return;

    auto delta = e.getPosition() - dragStart;
    if (!isDragging && (std::abs(delta.x) > 3 || std::abs(delta.y) > 3)) {
        isDragging = true;

        // CRITICAL FIX: Store original positions only once when drag starts
        storeOriginalNotePositions();
    }

    if (!isDragging) return;

    // Calculate delta from original drag start position
    double deltaTime = (e.x - dragStart.x) / xZoom;
    int deltaPitch = -((e.y - dragStart.y) / yZoom);

    // Apply snap to grid if enabled
    if (snapToGrid) {
        double gridSamples = (60.0 / bpm) / 4.0 * sampleRate;
        deltaTime = std::round(deltaTime / gridSamples) * gridSamples;
    }

    deltaPitch = (int)std::round(deltaPitch);

    if (isResizing && resizingMessage && resizingPartner) {
        handleResizeDrag(deltaTime);
    }
    else if (!selectedMessages.empty()) {
        handleMoveDrag(deltaTime, deltaPitch);
    }

    repaint();
}

void PianoRollComponent::mouseUp(const juce::MouseEvent& e)
{
    setMouseCursor(getCursorForTool(currentTool));

    bool wasResizing = isResizing;
    bool wasDragging = isDragging;

    isPanning = false;
    isBoxSelecting = false;
    drawSelectionRect = false;

    if (wasResizing && wasDragging && resizingMessage && resizingPartner) {
        finalizeResize();
    }

    if (wasDragging && !selectedMessages.empty() && currentTool == Tool::SELECT) {
        finalizeMove();
    }

    // Pencil-Drag abschliessen: Thumbnail final aktualisieren, State raeumen.
    if (isPencilDrawing) {
        if (midiRegion != nullptr) {
            midiRegion->rebuildThumbnail();
            midiRegion->updateThumb();
        }
        notifyRegionModified();
        isPencilDrawing = false;
        pencilNoteOn    = nullptr;
        pencilNoteOff   = nullptr;
    }

    // Reset all drag states
    resetDragState();
    repaint();
}

// =================== HELPER METHODS FOR SAFE DRAGGING ===================

void PianoRollComponent::storeOriginalNotePositions()
{
    originalNotePositions.clear();

    for (auto* msg : selectedMessages) {
        MidiMessage* noteOff = findNoteOffForNoteOn(msg);
        if (noteOff) {
            originalNotePositions[msg] = {
                msg->getTimeStamp(),
                msg->getNoteNumber(),
                noteOff->getTimeStamp()
            };
        }
    }
}

void PianoRollComponent::handleResizeDrag(double deltaTime)
{
    if (!resizingMessage || !resizingPartner) return;

    double newDuration = originalDuration + deltaTime;
    double minDuration = (60.0 / bpm) / 16.0 * sampleRate;
    newDuration = juce::jmax(minDuration, newDuration);

    // Create preview position without modifying original
    dragPreviewEndTime = resizingMessage->getTimeStamp() + newDuration;
}

void PianoRollComponent::handleMoveDrag(double deltaTime, int deltaPitch)
{
    // Clear previous preview
    dragPreviewNotes.clear();

    // Create preview positions without modifying originals
    for (auto* msg : selectedMessages) {
        auto it = originalNotePositions.find(msg);
        if (it != originalNotePositions.end()) {
            double originalStartTime = std::get<0>(it->second);
            int originalPitch = std::get<1>(it->second);
            double originalEndTime = std::get<2>(it->second);
            double originalDuration = originalEndTime - originalStartTime;

            // Calculate new position based on original + delta
            double newStartTime = juce::jmax(0.0, originalStartTime + deltaTime);
            int newPitch = juce::jlimit(0, 127, originalPitch + deltaPitch);

            // Store preview data
            DragPreviewNote preview;
            preview.originalNote = msg;
            preview.previewStartTime = newStartTime;
            preview.previewEndTime = newStartTime + originalDuration;
            preview.previewPitch = newPitch;

            dragPreviewNotes.push_back(preview);
        }
    }
}

void PianoRollComponent::finalizeResize()
{
    if (!resizingMessage || !resizingPartner) return;

    // Apply the preview changes to actual notes
    double minDuration = (60.0 / bpm) / 16.0 * sampleRate;
    double finalEndTime = dragPreviewEndTime;
    double currentDuration = finalEndTime - resizingMessage->getTimeStamp();

    if (currentDuration < minDuration) {
        finalEndTime = resizingMessage->getTimeStamp() + minDuration;
    }

    if (snapToGrid) {
        finalEndTime = snapTimeToGrid(finalEndTime);
    }

    // Now safely apply the change
    resizingPartner->setTimeStamp(finalEndTime);

    // Update region
    midiRegion->rebuildThumbnail();
    midiRegion->updateThumb();
    notifyRegionModified();
}

void PianoRollComponent::finalizeMove()
{
    if (dragPreviewNotes.empty()) return;

    // Apply all preview changes to actual notes
    for (const auto& preview : dragPreviewNotes) {
        MidiMessage* noteOff = findNoteOffForNoteOn(preview.originalNote);
        if (!noteOff) continue;

        double finalStartTime = preview.previewStartTime;
        double finalEndTime = preview.previewEndTime;
        int finalPitch = preview.previewPitch;

        if (snapToGrid) {
            finalStartTime = snapTimeToGrid(finalStartTime);
            double duration = finalEndTime - preview.previewStartTime;
            finalEndTime = finalStartTime + duration;
        }

        // Apply changes safely
        preview.originalNote->setTimeStamp(finalStartTime);
        preview.originalNote->setNoteNumber(finalPitch);
        noteOff->setTimeStamp(finalEndTime);
        noteOff->setNoteNumber(finalPitch);
    }

    // Update region
    midiRegion->rebuildThumbnail();
    midiRegion->updateThumb();
    notifyRegionModified();
}

void PianoRollComponent::resetDragState()
{
    isResizing = false;
    isVelocityEditing = false;
    isDragging = false;
    resizingMessage = nullptr;
    resizingPartner = nullptr;
    originalVelocities.clear();
    originalNotePositions.clear();
    dragPreviewNotes.clear();
    dragPreviewEndTime = 0.0;
}

void PianoRollComponent::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (currentTool != Tool::SELECT || e.getPosition().x <= pianoWidth) return;

    // Double-click to create note
    int pitch = getPitchAtY(e.y);
    double start = snapToGrid ? snapTimeToGrid(getTimeAtX(e.x)) : getTimeAtX(e.x);
    // Note-Laenge = ein Grid-Step (BPM-bewusst). Vorher: sampleRate *
    // gridResolution — das ist BPM-frei und korrekt nur bei 60 BPM.
    double duration = getGridTimeStep();

    pitch = snapPitchToScale(pitch);
    addNotePair(pitch, start, duration);

    notifyRegionModified();
    repaint();
}

void PianoRollComponent::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    // On macOS, Ctrl+Scroll is intercepted by the OS (accessibility zoom).
    // Use Cmd (Command) as the zoom modifier on macOS; Ctrl still works on Windows/Linux.
    const bool isZoomModifier = e.mods.isCommandDown() || e.mods.isCtrlDown();

    if (isZoomModifier) {
        float factor = 1.0f + wheel.deltaY;
        juce::Point<float> mousePos = e.getPosition().toFloat();

        if (e.mods.isAltDown()) {
            // Option+Cmd+Scroll: horizontal zoom anchored at mouse position
            double timeAtMouse = getTimeAtX((int)mousePos.x);
            xZoom *= factor;
            xZoom = juce::jlimit(0.001f, 2.0f, xZoom);
            int newX = getXForTime(timeAtMouse);
            panOffset.x += mousePos.x - newX;
        }
        else {
            // Cmd+Scroll: vertical zoom anchored at mouse position
            int pitchAtMouse = getPitchAtY((int)mousePos.y);
            yZoom *= factor;
            yZoom = juce::jlimit(4.0f, 64.0f, yZoom);
            int newY = getYForPitch(pitchAtMouse);
            panOffset.y += mousePos.y - newY;
        }
    }
    else if (e.mods.isShiftDown() || std::abs(wheel.deltaX) > std::abs(wheel.deltaY)) {
        // Horizontal scroll — Shift+Scroll (Windows) or natural trackpad deltaX (macOS)
        float dx = (std::abs(wheel.deltaX) > 0.0f) ? wheel.deltaX : wheel.deltaY;
        panOffset.x += dx * 50.0f;
        panOffset.x = juce::jmin(panOffset.x, 0.0f);
    }
    else {
        // Vertical scroll
        panOffset.y += wheel.deltaY * 50.0f;
    }

    repaint();
}

void PianoRollComponent::mouseMagnify(const juce::MouseEvent& e, float scaleFactor)
{
    // Trackpad pinch gesture
    xZoom *= scaleFactor;
    yZoom *= scaleFactor;

    xZoom = juce::jlimit(0.001f, 2.0f, xZoom);
    yZoom = juce::jlimit(4.0f, 64.0f, yZoom);

    repaint();
}

// =================== KEYBOARD HANDLING ===================

// Enhanced keyPressed method with cursor key navigation
bool PianoRollComponent::keyPressed(const juce::KeyPress& key, juce::Component*)
{
    auto keyCode = key.getKeyCode();
    bool shiftHeld = key.getModifiers().isShiftDown();
    bool cmdHeld = key.getModifiers().isCtrlDown();

    // Spacebar → play/stop
    if (key == juce::KeyPress::spaceKey) {
        if (onPlayToggle) onPlayToggle();
        return true;
    }

    // Tool shortcuts
    if (keyCode == '1') { setCurrentTool(Tool::SELECT); return true; }
    if (keyCode == '2') { setCurrentTool(Tool::PENCIL); return true; }
    if (keyCode == '3') { setCurrentTool(Tool::ERASER); return true; }
    if (keyCode == '4') { setCurrentTool(Tool::VELOCITY); return true; }
    if (keyCode == '5') { setCurrentTool(Tool::SPLIT); return true; }

    // Editing shortcuts
    if (keyCode == juce::KeyPress::deleteKey) {
        cutSelected();
        return true;
    }

    // Undo/Redo
    if (keyCode == 90 && cmdHeld) {
        if (shiftHeld) {
            performRedo();
        }
        else {
            performUndo();
        }
        return true;
    }

    // Selection shortcuts
    if (keyCode == 65 && cmdHeld) {
        selectAll();
        return true;
    }

    // Copy/Cut/Paste
    if (keyCode == 67 && cmdHeld) { copySelected(); return true; }
    if (keyCode == 88 && cmdHeld) { cutSelected(); return true; }
    if (keyCode == 86 && cmdHeld) {
        double pasteTime = playbackPosition >= 0.0 ? playbackPosition : 0.0;
        pasteAtPosition(pasteTime);
        return true;
    }

    // CURSOR KEY NAVIGATION - The new functionality!
    if (!selectedMessages.empty()) {
        if (keyCode == juce::KeyPress::upKey) {
            // Move up by semitone or octave
            int semitones = shiftHeld ? 12 : 1;
            moveSelectedNotes(0.0, semitones);
            return true;
        }

        if (keyCode == juce::KeyPress::downKey) {
            // Move down by semitone or octave
            int semitones = shiftHeld ? -12 : -1;
            moveSelectedNotes(0.0, semitones);
            return true;
        }

        if (keyCode == juce::KeyPress::leftKey) {
            // Move to previous grid point
            double gridTime = getGridTimeStep();
            moveSelectedNotes(-gridTime, 0);
            return true;
        }

        if (keyCode == juce::KeyPress::rightKey) {
            // Move to next grid point
            double gridTime = getGridTimeStep();
            moveSelectedNotes(gridTime, 0);
            return true;
        }
    }

    // Quantize shortcut
    if (keyCode == 81 && cmdHeld) {
        quantizeSelected();
        return true;
    }

    // Playback shortcuts
    if (keyCode == juce::KeyPress::spaceKey) {
        // Toggle playback - would be handled by parent component
        return false; // Let parent handle
    }

    // Note input via QWERTY keyboard
    static std::map<int, int> keyToNote = {
        {'a', 60}, {'w', 61}, {'s', 62}, {'e', 63}, {'d', 64}, {'f', 65},
        {'t', 66}, {'g', 67}, {'y', 68}, {'h', 69}, {'u', 70}, {'j', 71},
        {'k', 72}, {'o', 73}, {'l', 74}, {'p', 75}, {';', 76}
    };

    auto it = keyToNote.find(keyCode);
    if (it != keyToNote.end() && playbackPosition >= 0.0) {
        // Play note at current playback position
        int pitch = it->second;
        // Note-Laenge = ein Grid-Step (BPM-bewusst). Vorher: sampleRate *
    // gridResolution — das ist BPM-frei und korrekt nur bei 60 BPM.
    double duration = getGridTimeStep();
        addNotePair(pitch, playbackPosition, duration);
        notifyRegionModified();
        repaint();
        return true;
    }

    return false;
}

// Enhanced method to move selected notes with proper validation and undo support
void PianoRollComponent::moveSelectedNotes(double deltaTime, int deltaPitch)
{
    if (selectedMessages.empty() || midiRegion == nullptr) return;

    // Create undo action first
    std::vector<NotePositionData> originalPositions;

    // Store original positions for undo
    for (auto* msg : selectedMessages) {
        MidiMessage* noteOff = findNoteOffForNoteOn(msg);
        if (noteOff) {
            NotePositionData data;
            data.noteOn = msg;
            data.noteOff = noteOff;
            data.startTime = msg->getTimeStamp();
            data.endTime = noteOff->getTimeStamp();
            data.pitch = msg->getNoteNumber();
            originalPositions.push_back(data);
        }
    }

    // Validate and apply moves
    bool anyMoved = false;
    std::vector<NotePositionData> newPositions;

    for (auto* msg : selectedMessages) {
        MidiMessage* noteOff = findNoteOffForNoteOn(msg);
        if (!noteOff) continue;

        double originalStart = msg->getTimeStamp();
        double originalEnd = noteOff->getTimeStamp();
        double duration = originalEnd - originalStart;
        int originalPitch = msg->getNoteNumber();

        // Calculate new positions
        double newStartTime = originalStart + deltaTime;
        int newPitch = originalPitch + deltaPitch;

        // Validate new positions
        newStartTime = juce::jmax(0.0, newStartTime);  // Don't go before start
        newPitch = juce::jlimit(0, 127, newPitch);     // Stay in MIDI range

        // Apply snap to grid for time movement
        if (snapToGrid && deltaTime != 0.0) {
            newStartTime = snapTimeToGrid(newStartTime);
        }

        // Pitch-Snap zur Skala wird hier bewusst NICHT angewandt: Cursor-Tasten
        // sind eine explizite "transponiere um genau N Halbtoene"-Geste. Bei
        // C-Dur als customScale wuerde C->C# (out of scale) auf den naechsten
        // In-Scale-Ton snappen — und dabei wegen der Tie-Break-Reihenfolge auf
        // C zurueckfallen, wodurch sich beim Pfeil-nach-oben gar nichts mehr
        // bewegt. Tonleiter-Constraint gehoert in den Mauspfad (Pencil/Drag),
        // nicht in die explizite Tastatursteuerung.

        // Check if position actually changed
        if (newStartTime != originalStart || newPitch != originalPitch) {
            // Apply the move
            msg->setTimeStamp(newStartTime);
            msg->setNoteNumber(newPitch);
            noteOff->setTimeStamp(newStartTime + duration);
            noteOff->setNoteNumber(newPitch);

            // Store new position for undo system
            NotePositionData newData;
            newData.noteOn = msg;
            newData.noteOff = noteOff;
            newData.startTime = newStartTime;
            newData.endTime = newStartTime + duration;
            newData.pitch = newPitch;
            newPositions.push_back(newData);

            anyMoved = true;
        }
    }

    if (anyMoved) {
        // Create undo action
        pushUndoAction("Move Notes",
            [this, originalPositions]() {
                // Undo: restore original positions
                for (const auto& data : originalPositions) {
                    data.noteOn->setTimeStamp(data.startTime);
                    data.noteOn->setNoteNumber(data.pitch);
                    data.noteOff->setTimeStamp(data.endTime);
                    data.noteOff->setNoteNumber(data.pitch);
                }
                midiRegion->rebuildThumbnail();
                midiRegion->updateThumb();
                repaint();
            },
            [this, newPositions]() {
                // Redo: apply new positions
                for (const auto& data : newPositions) {
                    data.noteOn->setTimeStamp(data.startTime);
                    data.noteOn->setNoteNumber(data.pitch);
                    data.noteOff->setTimeStamp(data.endTime);
                    data.noteOff->setNoteNumber(data.pitch);
                }
                midiRegion->rebuildThumbnail();
                midiRegion->updateThumb();
                repaint();
            });

        // Update MIDI region
        midiRegion->rebuildThumbnail();
        midiRegion->updateThumb();
        notifyRegionModified();
        repaint();

        // Auto-scroll to keep selection visible if needed
        ensureSelectionVisible();
    }
}

// Helper method to calculate grid time step
double PianoRollComponent::getGridTimeStep()
{
    // Calculate time for one grid step based on current grid resolution
    double quarterNoteSamples = (60.0 / bpm) * sampleRate;  // Samples per quarter note
    return quarterNoteSamples * gridResolution;  // Grid step in samples
}

// Helper method to ensure selection stays visible after keyboard moves
void PianoRollComponent::ensureSelectionVisible()
{
    if (selectedMessages.empty()) return;

    // Find bounds of selected notes
    double minTime = std::numeric_limits<double>::max();
    double maxTime = std::numeric_limits<double>::lowest();
    int minPitch = 127;
    int maxPitch = 0;

    for (auto* msg : selectedMessages) {
        MidiMessage* noteOff = findNoteOffForNoteOn(msg);
        if (noteOff) {
            minTime = juce::jmin(minTime, msg->getTimeStamp());
            maxTime = juce::jmax(maxTime, noteOff->getTimeStamp());
            minPitch = juce::jmin(minPitch, msg->getNoteNumber());
            maxPitch = juce::jmax(maxPitch, msg->getNoteNumber());
        }
    }

    // Check if selection is visible horizontally
    int leftX = getXForTime(minTime);
    int rightX = getXForTime(maxTime);
    int viewLeft = pianoWidth;
    int viewRight = getWidth();

    if (leftX < viewLeft || rightX > viewRight) {
        // Adjust horizontal pan to center the selection
        double centerTime = (minTime + maxTime) / 2.0;
        int centerX = getXForTime(centerTime);
        int viewCenterX = (viewLeft + viewRight) / 2;

        panOffset.x += viewCenterX - centerX;
        panOffset.x = juce::jmin(panOffset.x, 0.0f);
    }

    // Check if selection is visible vertically
    int topY = getYForPitch(maxPitch);
    int bottomY = getYForPitch(minPitch);
    int viewTop = 0;
    int viewBottom = getHeight();

    if (topY < viewTop || bottomY > viewBottom) {
        // Adjust vertical pan to center the selection
        int centerPitch = (minPitch + maxPitch) / 2;
        int centerY = getYForPitch(centerPitch);
        int viewCenterY = (viewTop + viewBottom) / 2;

        panOffset.y += viewCenterY - centerY;
    }

    repaint();
}

// Add undo/redo methods if not already present
void PianoRollComponent::performUndo()
{
    if (!undoStack.empty()) {
        auto action = std::move(undoStack.top());
        undoStack.pop();

        if (action->undoFunc) {
            action->undoFunc();
        }

        redoStack.push(std::move(action));
    }
}

void PianoRollComponent::performRedo()
{
    if (!redoStack.empty()) {
        auto action = std::move(redoStack.top());
        redoStack.pop();

        if (action->redoFunc) {
            action->redoFunc();
        }

        undoStack.push(std::move(action));
    }
}


void PianoRollComponent::modifierKeysChanged(const juce::ModifierKeys& modifiers)
{
    // Update cursor based on modifiers and current tool
    setMouseCursor(getCursorForTool(currentTool));
    repaint();
}

// =================== HELPER METHODS ===================

MidiMessage* PianoRollComponent::findNoteOffForNoteOn(MidiMessage* noteOn)
{
    if (!noteOn || !noteOn->isNoteOn() || midiRegion == nullptr) return nullptr;

    auto& messages = midiRegion->getMidiMessageList();
    const int channel = noteOn->getChannel();
    const int noteNumber = noteOn->getNoteNumber();

    // FIFO-Pairing: collect all noteOns and noteOffs at the same channel+pitch,
    // sort each list by timestamp, then pair the k-th noteOn with the k-th noteOff.
    // The previous "earliest noteOff after this noteOn" heuristic broke as soon as
    // two notes shared a pitch — e.g. when a moved note steps onto an existing one,
    // the other note's noteOff was picked and got mutated by the next move.
    std::vector<MidiMessage*> ons, offs;
    ons.reserve(messages.size());
    offs.reserve(messages.size());

    for (auto* msg : messages) {
        if (!msg) continue;
        if (msg->getChannel() != channel || msg->getNoteNumber() != noteNumber) continue;
        if (msg->isNoteOn())       ons.push_back(msg);
        else if (msg->isNoteOff()) offs.push_back(msg);
    }

    auto byTime = [](MidiMessage* a, MidiMessage* b) {
        return a->getTimeStamp() < b->getTimeStamp();
    };
    std::stable_sort(ons.begin(),  ons.end(),  byTime);
    std::stable_sort(offs.begin(), offs.end(), byTime);

    auto it = std::find(ons.begin(), ons.end(), noteOn);
    if (it == ons.end()) return nullptr;

    const size_t idx = (size_t)std::distance(ons.begin(), it);
    return idx < offs.size() ? offs[idx] : nullptr;
}
double PianoRollComponent::getNoteDuration(MidiMessage* noteOn)
{
    MidiMessage* noteOff = findNoteOffForNoteOn(noteOn);
    return noteOff ? (noteOff->getTimeStamp() - noteOn->getTimeStamp()) : 0.0;
}


juce::MidiMessage* PianoRollComponent::addNotePair(int pitch, double startTime, double duration, int velocity, int channel)
{
    if (midiRegion == nullptr) return nullptr;
    if (snapToGrid) {
        startTime = snapTimeToGrid(startTime);

        // Align duration to grid as well
        double gridSamples = (60.0 / bpm) / 4.0 * sampleRate; // 16th note grid
        double gridDuration = std::round(duration / gridSamples) * gridSamples;
        duration = juce::jmax(gridSamples / 4.0, gridDuration); // Minimum 1/64 note
    }

    // Ensure minimum duration for playback
    double minDuration = sampleRate * 0.01; // 10ms minimum
    duration = juce::jmax(minDuration, duration);

    MidiMessage* on = new MidiMessage(MidiMessage::noteOn(channel, pitch, (juce::uint8)velocity));
    on->setTimeStamp(startTime);

    MidiMessage* off = new MidiMessage(MidiMessage::noteOff(channel, pitch));
    off->setTimeStamp(startTime + duration);

    midiRegion->getMidiMessageList().push_back(on);
    midiRegion->getMidiMessageList().push_back(off);

    // Fix: Sort messages by timestamp for proper playback
    auto& messages = midiRegion->getMidiMessageList();
    std::sort(messages.begin(), messages.end(),
        [](const MidiMessage* a, const MidiMessage* b) {
            return a->getTimeStamp() < b->getTimeStamp();
        });

    midiRegion->rebuildThumbnail();
    midiRegion->updateThumb();
    return on;
}

void PianoRollComponent::deleteNotePair(MidiMessage* noteOn)
{
    if (noteOn == nullptr || midiRegion == nullptr) return;
    MidiMessage* noteOff = findNoteOffForNoteOn(noteOn);

    // Create undo action
    if (noteOn && noteOff) {
        MidiMessage onCopy(*noteOn);
        MidiMessage offCopy(*noteOff);

        pushUndoAction("Delete Note",
            [this, onCopy, offCopy]() {
                // Undo: restore notes from value copies
                if (midiRegion == nullptr) return;
                MidiMessage* newOn = new MidiMessage(onCopy);
                MidiMessage* newOff = new MidiMessage(offCopy);
                midiRegion->getMidiMessageList().push_back(newOn);
                midiRegion->getMidiMessageList().push_back(newOff);
                midiRegion->rebuildThumbnail();
                midiRegion->updateThumb();
            },
            [this, onCopy, offCopy]() {
                // Redo: find by value and delete — do NOT use the original pointers,
                // they were freed during perform() and would be dangling here.
                if (midiRegion == nullptr) return;
                auto& messages = midiRegion->getMidiMessageList();
                auto removeByValue = [&](const MidiMessage& target, bool wantNoteOn) {
                    auto it = std::find_if(messages.begin(), messages.end(),
                        [&](MidiMessage* m) {
                            return m != nullptr
                                && (wantNoteOn ? m->isNoteOn() : m->isNoteOff())
                                && m->getNoteNumber() == target.getNoteNumber()
                                && m->getChannel()    == target.getChannel()
                                && m->getTimeStamp()  == target.getTimeStamp();
                        });
                    if (it != messages.end()) { delete *it; messages.erase(it); }
                };
                removeByValue(onCopy, true);
                removeByValue(offCopy, false);
                midiRegion->rebuildThumbnail();
                midiRegion->updateThumb();
            });
    }

    // Remove from selection before freeing to prevent stale pointers in selectedMessages
    selectedMessages.erase(noteOn);

    auto& messages = midiRegion->getMidiMessageList();
    messages.erase(std::remove(messages.begin(), messages.end(), noteOn), messages.end());
    if (noteOff) {
        messages.erase(std::remove(messages.begin(), messages.end(), noteOff), messages.end());
        delete noteOff;
    }
    delete noteOn;

    midiRegion->rebuildThumbnail();
    midiRegion->updateThumb();
}

void PianoRollComponent::splitNote(MidiMessage* noteOn, double splitTime)
{
    if (!noteOn) return;

    MidiMessage* noteOff = findNoteOffForNoteOn(noteOn);
    if (!noteOff) return;

    double originalStart = noteOn->getTimeStamp();
    double originalEnd = noteOff->getTimeStamp();

    if (splitTime <= originalStart || splitTime >= originalEnd) return;

    int pitch = noteOn->getNoteNumber();
    int velocity = noteOn->getVelocity();
    int channel = noteOn->getChannel();

    // Modify first note to end at split
    noteOff->setTimeStamp(splitTime);

    // Create second note from split to original end
    addNotePair(pitch, splitTime, originalEnd - splitTime, velocity, channel);
}

// Color and visual helpers
juce::Colour PianoRollComponent::getColourForVelocity(int velocity)
{
    float hue = juce::jmap((float)velocity, 0.0f, 127.0f, 0.8f, 0.0f); // Blue to red
    return juce::Colour::fromHSV(hue, 0.8f, 0.9f, 1.0f);
}

juce::Colour PianoRollComponent::getColourForChannel(int channel)
{
    return noteColours[channel % 16];
}

juce::MouseCursor PianoRollComponent::getCursorForTool(Tool tool)
{
    switch (tool) {
    case Tool::SELECT:   return juce::MouseCursor::NormalCursor;
    case Tool::PENCIL:   return juce::MouseCursor::CrosshairCursor;
    case Tool::ERASER:   return juce::MouseCursor::NormalCursor; // Could use custom eraser cursor
    case Tool::VELOCITY: return juce::MouseCursor::UpDownResizeCursor;
    case Tool::SPLIT:    return juce::MouseCursor::CrosshairCursor;
    default:             return juce::MouseCursor::NormalCursor;
    }
}

// More helper implementations...
bool PianoRollComponent::isBlackKey(int n)
{
    int note = n % 12;
    return note == 1 || note == 3 || note == 6 || note == 8 || note == 10;
}

int PianoRollComponent::getYForPitch(int pitch)
{
    return (int)(((127 - pitch) * yZoom) + panOffset.y);
}

int PianoRollComponent::getPitchAtY(int y)
{
    return 127 - (int)((y - panOffset.y) / yZoom);
}

int PianoRollComponent::getXForTime(double sampleTime)
{
    // sampleTime ist BUFFER-relativ (gleiche Zeitachse wie midiMessageList-
    // Timestamps). Der Piano-Roll zeigt nur den sichtbaren Ausschnitt der
    // Region — bei links-getrimmten Regionen verschiebt sourceStartSample die
    // Achse: x=pianoWidth entspricht buffer-time = sourceStartSample.
    const double base = midiRegion != nullptr ? (double) midiRegion->getSourceStartSample() : 0.0;
    return pianoWidth + (int)((sampleTime - base) * xZoom + panOffset.x);
}

double PianoRollComponent::getTimeAtX(int x)
{
    const double base = midiRegion != nullptr ? (double) midiRegion->getSourceStartSample() : 0.0;
    return base + (x - pianoWidth - panOffset.x) / xZoom;
}

double PianoRollComponent::snapTimeToGrid(double time)
{
    if (!snapToGrid) return time;

    double gridSamples = (60.0 / bpm) * sampleRate * gridResolution;
    return std::round(time / gridSamples) * gridSamples;
}

int PianoRollComponent::snapPitchToScale(int pitch)
{
    if (customScale.empty()) return pitch;

    int octave = pitch / 12;
    int note = pitch % 12;

    // Find closest scale note
    int closestNote = note;
    int minDistance = 12;

    for (int scaleNote : customScale) {
        int distance = std::abs(note - scaleNote);
        if (distance < minDistance) {
            minDistance = distance;
            closestNote = scaleNote;
        }
    }

    return octave * 12 + closestNote;
}

juce::Rectangle<int> PianoRollComponent::getNoteRect(const juce::MidiMessage& msg, double duration)
{
    int x = getXForTime(msg.getTimeStamp());
    int y = getYForPitch(msg.getNoteNumber());
    int w = juce::jmax(2, (int)(duration * xZoom));
    int h = juce::jmax(2, (int)yZoom - 1);
    return { x, y, w, h };
}

juce::Rectangle<int> PianoRollComponent::getVelocityRect(const juce::MidiMessage& msg, double duration)
{
    auto noteRect = getNoteRect(msg, duration);
    int velHeight = (int)(msg.getVelocity() / 127.0f * controllerLaneHeight);
    return { noteRect.getX(), getHeight() - velHeight, noteRect.getWidth(), velHeight };
}

// Hit testing
MidiMessage* PianoRollComponent::getNoteAt(juce::Point<int> position)
{
    if (midiRegion == nullptr) return nullptr;
    auto& messages = midiRegion->getMidiMessageList();

    for (auto* msg : messages) {
        if (!msg || !msg->isNoteOn()) continue;

        double duration = getNoteDuration(msg);
        if (duration <= 0.0) continue;

        auto rect = getNoteRect(*msg, duration);
        if (rect.contains(position)) {
            return msg;
        }
    }
    return nullptr;
}

std::vector<MidiMessage*> PianoRollComponent::getNotesInRect(juce::Rectangle<int> rect)
{
    std::vector<MidiMessage*> result;
    if (midiRegion == nullptr) return result;
    auto& messages = midiRegion->getMidiMessageList();

    for (auto* msg : messages) {
        if (!msg || !msg->isNoteOn()) continue;

        double duration = getNoteDuration(msg);
        if (duration <= 0.0) continue;

        auto noteRect = getNoteRect(*msg, duration);
        if (rect.intersects(noteRect)) {
            result.push_back(msg);
        }
    }
    return result;
}

bool PianoRollComponent::isOverNoteEdge(juce::Point<int> position, MidiMessage* note, double duration)
{
    auto rect = getNoteRect(*note, duration);
    return std::abs(position.x - rect.getRight()) < 8;
}

// High-level operations
void PianoRollComponent::selectAll()
{
    selectedMessages.clear();
    if (midiRegion == nullptr) return;
    auto& messages = midiRegion->getMidiMessageList();

    for (auto* msg : messages) {
        if (msg && msg->isNoteOn()) {
            selectedMessages.insert(msg);
        }
    }

    notifySelectionChanged();
    repaint();
}

void PianoRollComponent::selectNone()
{
    selectedMessages.clear();
    notifySelectionChanged();
    repaint();
}

void PianoRollComponent::quantizeSelected(int gridDivision)
{
    if (selectedMessages.empty()) return;

    // gridDivision = N steht fuer 1/N-Note. Eine 1/N-Note ist 4/N einer
    // Viertelnote — analog zur Konvention im Rest der Datei (gridResolution
    // ist 4/N). Vorher stand hier nur "/ gridDivision", was bei N=16 in
    // Wahrheit auf 1/64 quantisiert hat.
    if (gridDivision <= 0) return;
    double gridSamples = (60.0 / bpm) * sampleRate * 4.0 / (double) gridDivision;

    for (auto* msg : selectedMessages) {
        MidiMessage* noteOff = findNoteOffForNoteOn(msg);
        if (!noteOff) continue;

        double originalStart = msg->getTimeStamp();
        double originalEnd = noteOff->getTimeStamp();
        double duration = originalEnd - originalStart;

        double quantizedStart = std::round(originalStart / gridSamples) * gridSamples;
        msg->setTimeStamp(quantizedStart);
        noteOff->setTimeStamp(quantizedStart + duration);
    }

    midiRegion->rebuildThumbnail();
    midiRegion->updateThumb();
    notifyRegionModified();
    repaint();
}

void PianoRollComponent::setSelectedVelocity(int velocity)
{
    velocity = juce::jlimit(1, 127, velocity);

    for (auto* msg : selectedMessages) {
        if (onVelocityChanged) {
            onVelocityChanged(msg, msg->getVelocity(), velocity);
        }
        msg->setVelocity(velocity);
    }

    notifyRegionModified();
    repaint();
}

void PianoRollComponent::transposeSelected(int semitones)
{
    for (auto* msg : selectedMessages) {
        MidiMessage* noteOff = findNoteOffForNoteOn(msg);
        if (!noteOff) continue;

        int newPitch = juce::jlimit(0, 127, msg->getNoteNumber() + semitones);
        msg->setNoteNumber(newPitch);
        noteOff->setNoteNumber(newPitch);
    }

    midiRegion->rebuildThumbnail();
    midiRegion->updateThumb();
    notifyRegionModified();
    repaint();
}

// Undo/Redo system
void PianoRollComponent::pushUndoAction(const juce::String& description,
    std::function<void()> undoFunc,
    std::function<void()> redoFunc)
{
    auto action = std::make_unique<UndoableAction>();
    action->description = description;
    action->undoFunc = undoFunc;
    action->redoFunc = redoFunc;

    undoStack.push(std::move(action));

    // Clear redo stack
    while (!redoStack.empty()) {
        redoStack.pop();
    }

    // Limit undo stack size
    const int maxUndoSteps = 100;
    if (undoStack.size() > maxUndoSteps) {
        // Remove oldest undo actions
        std::stack<std::unique_ptr<UndoableAction>> tempStack;
        for (int i = 0; i < maxUndoSteps - 1; ++i) {
            if (!undoStack.empty()) {
                tempStack.push(std::move(undoStack.top()));
                undoStack.pop();
            }
        }

        // Clear remaining and restore
        while (!undoStack.empty()) undoStack.pop();
        while (!tempStack.empty()) {
            undoStack.push(std::move(tempStack.top()));
            tempStack.pop();
        }
    }
}

// Notification methods
void PianoRollComponent::notifySelectionChanged()
{
    if (onSelectionChanged) {
        onSelectionChanged(selectedMessages);
    }
}

void PianoRollComponent::notifyRegionModified()
{
    if (onRegionModified) {
        onRegionModified();
    }
}

// =================== CLIPBOARD OPERATIONS ===================

void PianoRollComponent::copySelected()
{
    if (selectedMessages.empty()) return;

    clipboard.clear();

    // Find the earliest start time for relative positioning
    double earliestTime = std::numeric_limits<double>::max();
    for (auto* msg : selectedMessages) {
        earliestTime = juce::jmin(earliestTime, msg->getTimeStamp());
    }
    clipboardTimeOrigin = earliestTime;

    // Copy selected notes to clipboard
    for (auto* msg : selectedMessages) {
        ClipboardNote note;
        note.pitch = msg->getNoteNumber();
        note.startTime = msg->getTimeStamp() - earliestTime; // Relative time
        note.duration = getNoteDuration(msg);
        note.velocity = msg->getVelocity();
        note.channel = msg->getChannel();

        if (note.duration > 0.0) {
            clipboard.push_back(note);
        }
    }
}

void PianoRollComponent::cutSelected()
{
    if (selectedMessages.empty()) return;

    copySelected();

    // Create undo action for cut
    std::vector<std::pair<MidiMessage, MidiMessage>> deletedNotes;
    for (auto* noteOn : selectedMessages) {
        MidiMessage* noteOff = findNoteOffForNoteOn(noteOn);
        if (noteOff) {
            deletedNotes.emplace_back(*noteOn, *noteOff);
        }
    }

    pushUndoAction("Cut Notes",
        [this, deletedNotes]() {
            // Undo: restore notes
            for (const auto& [on, off] : deletedNotes) {
                MidiMessage* newOn = new MidiMessage(on);
                MidiMessage* newOff = new MidiMessage(off);
                midiRegion->getMidiMessageList().push_back(newOn);
                midiRegion->getMidiMessageList().push_back(newOff);
            }
            midiRegion->rebuildThumbnail();
            midiRegion->updateThumb();
        },
        [this, deletedNotes]() {
            // Redo: delete notes again
            auto& messages = midiRegion->getMidiMessageList();
            for (const auto& [on, off] : deletedNotes) {
                messages.erase(std::remove_if(messages.begin(), messages.end(),
                    [&](MidiMessage* msg) {
                        return msg && (
                            (msg->isNoteOn() && msg->getNoteNumber() == on.getNoteNumber() &&
                                std::abs(msg->getTimeStamp() - on.getTimeStamp()) < 1.0) ||
                            (msg->isNoteOff() && msg->getNoteNumber() == off.getNoteNumber() &&
                                std::abs(msg->getTimeStamp() - off.getTimeStamp()) < 1.0)
                            );
                    }), messages.end());
            }
            midiRegion->rebuildThumbnail();
            midiRegion->updateThumb();
        });

    // Delete selected notes
    std::vector<MidiMessage*> toDelete;
    for (auto* msg : selectedMessages) {
        toDelete.push_back(msg);
        MidiMessage* noteOff = findNoteOffForNoteOn(msg);
        if (noteOff) toDelete.push_back(noteOff);
    }

    auto& messages = midiRegion->getMidiMessageList();
    for (auto* msg : toDelete) {
        messages.erase(std::remove(messages.begin(), messages.end(), msg), messages.end());
        delete msg;
    }

    selectedMessages.clear();
    midiRegion->rebuildThumbnail();
    midiRegion->updateThumb();

    notifySelectionChanged();
    notifyRegionModified();
    repaint();
}

void PianoRollComponent::pasteAtPosition(double timePosition, int pitch)
{
    if (clipboard.empty()) return;

    // If pitch is -1, use original pitches
    bool preservePitch = (pitch == -1);

    std::vector<MidiMessage*> newNotes;

    for (const auto& clipNote : clipboard) {
        int targetPitch = preservePitch ? clipNote.pitch :
            juce::jlimit(0, 127, pitch + (clipNote.pitch - clipboard[0].pitch));

        double startTime = timePosition + clipNote.startTime;

        MidiMessage* noteOn = new MidiMessage(
            MidiMessage::noteOn(clipNote.channel, targetPitch, (juce::uint8)clipNote.velocity));
        noteOn->setTimeStamp(startTime);

        MidiMessage* noteOff = new MidiMessage(
            MidiMessage::noteOff(clipNote.channel, targetPitch));
        noteOff->setTimeStamp(startTime + clipNote.duration);

        newNotes.push_back(noteOn);
        newNotes.push_back(noteOff);

        midiRegion->getMidiMessageList().push_back(noteOn);
        midiRegion->getMidiMessageList().push_back(noteOff);
    }

    // Create undo action
    pushUndoAction("Paste Notes",
        [this, newNotes]() {
            // Undo: remove pasted notes
            auto& messages = midiRegion->getMidiMessageList();
            for (auto* msg : newNotes) {
                messages.erase(std::remove(messages.begin(), messages.end(), msg), messages.end());
                delete msg;
            }
            midiRegion->rebuildThumbnail();
            midiRegion->updateThumb();
        },
        [this, newNotes]() {
            // Redo: add notes back (would need to recreate them)
            // This is complex - in practice you'd store the note data
            midiRegion->rebuildThumbnail();
            midiRegion->updateThumb();
        });

    midiRegion->rebuildThumbnail();
    midiRegion->updateThumb();
    notifyRegionModified();
    repaint();
}

// =================== ADVANCED OPERATIONS ===================

void PianoRollComponent::humanizeSelected(float amount)
{
    if (selectedMessages.empty()) return;

    juce::Random random(juce::Time::currentTimeMillis());
    double maxOffset = (60.0 / bpm) * sampleRate * amount * 0.1; // Max 10% of beat

    for (auto* msg : selectedMessages) {
        MidiMessage* noteOff = findNoteOffForNoteOn(msg);
        if (!noteOff) continue;

        // Randomize timing
        double timeOffset = random.nextFloat() * maxOffset * 2.0 - maxOffset;
        double originalStart = msg->getTimeStamp();
        double originalEnd = noteOff->getTimeStamp();
        double duration = originalEnd - originalStart;

        msg->setTimeStamp(juce::jmax(0.0, originalStart + timeOffset));
        noteOff->setTimeStamp(msg->getTimeStamp() + duration);

        // Randomize velocity slightly
        int velocityOffset = (int)(random.nextFloat() * 20.0f - 10.0f);
        int newVel = juce::jlimit(1, 127, msg->getVelocity() + velocityOffset);
        msg->setVelocity(newVel);
    }

    midiRegion->rebuildThumbnail();
    midiRegion->updateThumb();
    notifyRegionModified();
    repaint();
}

void PianoRollComponent::scaleSelectedVelocity(float factor)
{
    for (auto* msg : selectedMessages) {
        int newVel = juce::jlimit(1, 127, (int)(msg->getVelocity() * factor));
        msg->setVelocity(newVel);
    }

    notifyRegionModified();
    repaint();
}

void PianoRollComponent::randomizeSelectedVelocity(int range)
{
    juce::Random random(juce::Time::currentTimeMillis());

    for (auto* msg : selectedMessages) {
        int offset = (int)(random.nextFloat() * range * 2.0f - range);
        int newVel = juce::jlimit(1, 127, msg->getVelocity() + offset);
        msg->setVelocity(newVel);
    }

    notifyRegionModified();
    repaint();
}

void PianoRollComponent::crescendoSelected(int startVel, int endVel)
{
    if (selectedMessages.empty()) return;

    // Sort notes by time
    std::vector<MidiMessage*> sortedNotes(selectedMessages.begin(), selectedMessages.end());
    std::sort(sortedNotes.begin(), sortedNotes.end(),
        [](MidiMessage* a, MidiMessage* b) {
            return a->getTimeStamp() < b->getTimeStamp();
        });

    for (size_t i = 0; i < sortedNotes.size(); ++i) {
        float progress = sortedNotes.size() > 1 ? i / (float)(sortedNotes.size() - 1) : 0.0f;
        int vel = (int)(startVel + progress * (endVel - startVel));
        sortedNotes[i]->setVelocity(juce::jlimit(1, 127, vel));
    }

    notifyRegionModified();
    repaint();
}

void PianoRollComponent::stretchSelectedTempo(float factor)
{
    if (selectedMessages.empty() || factor <= 0.0f) return;

    // Find the earliest note as anchor point
    double anchorTime = std::numeric_limits<double>::max();
    for (auto* msg : selectedMessages) {
        anchorTime = juce::jmin(anchorTime, msg->getTimeStamp());
    }

    for (auto* msg : selectedMessages) {
        MidiMessage* noteOff = findNoteOffForNoteOn(msg);
        if (!noteOff) continue;

        double originalStart = msg->getTimeStamp();
        double originalEnd = noteOff->getTimeStamp();
        double duration = originalEnd - originalStart;

        // Stretch relative to anchor point
        double relativeStart = originalStart - anchorTime;
        double newStart = anchorTime + relativeStart * factor;
        double newDuration = duration * factor;

        msg->setTimeStamp(newStart);
        noteOff->setTimeStamp(newStart + newDuration);
    }

    midiRegion->rebuildThumbnail();
    midiRegion->updateThumb();
    notifyRegionModified();
    repaint();
}

void PianoRollComponent::reverseSelected()
{
    if (selectedMessages.empty()) return;

    // Find time bounds
    double minTime = std::numeric_limits<double>::max();
    double maxTime = std::numeric_limits<double>::lowest();

    for (auto* msg : selectedMessages) {
        MidiMessage* noteOff = findNoteOffForNoteOn(msg);
        if (!noteOff) continue;

        minTime = juce::jmin(minTime, msg->getTimeStamp());
        maxTime = juce::jmax(maxTime, noteOff->getTimeStamp());
    }

    // Reverse note positions
    for (auto* msg : selectedMessages) {
        MidiMessage* noteOff = findNoteOffForNoteOn(msg);
        if (!noteOff) continue;

        double originalStart = msg->getTimeStamp();
        double originalEnd = noteOff->getTimeStamp();
        double duration = originalEnd - originalStart;

        double newEnd = minTime + (maxTime - originalStart);
        double newStart = newEnd - duration;

        msg->setTimeStamp(newStart);
        noteOff->setTimeStamp(newEnd);
    }

    midiRegion->rebuildThumbnail();
    midiRegion->updateThumb();
    notifyRegionModified();
    repaint();
}

void PianoRollComponent::duplicateSelected(int times, double interval)
{
    if (selectedMessages.empty() || times <= 0) return;

    // If interval is 0, use the length of selected region
    if (interval == 0.0) {
        double minTime = std::numeric_limits<double>::max();
        double maxTime = std::numeric_limits<double>::lowest();

        for (auto* msg : selectedMessages) {
            MidiMessage* noteOff = findNoteOffForNoteOn(msg);
            if (!noteOff) continue;

            minTime = juce::jmin(minTime, msg->getTimeStamp());
            maxTime = juce::jmax(maxTime, noteOff->getTimeStamp());
        }
        interval = maxTime - minTime;
    }

    std::vector<MidiMessage*> allNewNotes;

    for (int i = 1; i <= times; ++i) {
        double timeOffset = i * interval;

        for (auto* originalMsg : selectedMessages) {
            MidiMessage* originalOff = findNoteOffForNoteOn(originalMsg);
            if (!originalOff) continue;

            double duration = originalOff->getTimeStamp() - originalMsg->getTimeStamp();

            MidiMessage* newOn = new MidiMessage(*originalMsg);
            newOn->setTimeStamp(originalMsg->getTimeStamp() + timeOffset);

            MidiMessage* newOff = new MidiMessage(*originalOff);
            newOff->setTimeStamp(originalOff->getTimeStamp() + timeOffset);

            allNewNotes.push_back(newOn);
            allNewNotes.push_back(newOff);

            midiRegion->getMidiMessageList().push_back(newOn);
            midiRegion->getMidiMessageList().push_back(newOff);
        }
    }

    // Create undo action
    pushUndoAction("Duplicate Notes",
        [this, allNewNotes]() {
            // Undo: remove duplicated notes
            auto& messages = midiRegion->getMidiMessageList();
            for (auto* msg : allNewNotes) {
                messages.erase(std::remove(messages.begin(), messages.end(), msg), messages.end());
                delete msg;
            }
            midiRegion->rebuildThumbnail();
            midiRegion->updateThumb();
        },
        []() {
            // Redo would need to recreate the notes
        });

    midiRegion->rebuildThumbnail();
    midiRegion->updateThumb();
    notifyRegionModified();
    repaint();
}

// =================== CHORD & SCALE OPERATIONS ===================

void PianoRollComponent::quantizeToScale(const std::vector<int>& scale)
{
    if (selectedMessages.empty() || scale.empty()) return;

    for (auto* msg : selectedMessages) {
        MidiMessage* noteOff = findNoteOffForNoteOn(msg);
        if (!noteOff) continue;

        int originalPitch = msg->getNoteNumber();
        int octave = originalPitch / 12;
        int note = originalPitch % 12;

        // Find closest scale note
        int closestNote = note;
        int minDistance = 12;

        for (int scaleNote : scale) {
            int distance = std::abs(note - scaleNote);
            if (distance < minDistance) {
                minDistance = distance;
                closestNote = scaleNote;
            }
        }

        int newPitch = octave * 12 + closestNote;
        newPitch = juce::jlimit(0, 127, newPitch);

        msg->setNoteNumber(newPitch);
        noteOff->setNoteNumber(newPitch);
    }

    midiRegion->rebuildThumbnail();
    midiRegion->updateThumb();
    notifyRegionModified();
    repaint();
}

void PianoRollComponent::addChord(int rootNote, double startTime, double duration,
    const std::vector<int>& intervals)
{
    for (int interval : intervals) {
        int pitch = juce::jlimit(0, 127, rootNote + interval);
        addNotePair(pitch, startTime, duration);
    }

    notifyRegionModified();
    repaint();
}

void PianoRollComponent::insertPattern(Pattern p)
{
    if (midiRegion == nullptr) return;

    // GM-Drum-Pitches. Channel = aktueller Track-Kanal: TrackIn filtert
    // beim Playback auf Track::getMidiChannel(); ein hartcodierter Kanal 10
    // wuerde sonst geschluckt, sobald der Track auf einem anderen Kanal
    // liegt (Default ist 1). Der Nutzer wechselt seinen Drum-Track ggf.
    // selbst auf 10, GM-Pitches passen dann auch klanglich.
    constexpr int kKick       = 36;
    constexpr int kSnare      = 38;
    constexpr int kClap       = 39;
    constexpr int kClosedHat  = 42;
    constexpr int kOpenHat    = 46;
    constexpr int kPedalHat   = 44;
    constexpr int kRide       = 51;
    constexpr int kCrash      = 49;
    constexpr int kRimShot    = 37;
    constexpr int kTomLow     = 41;
    constexpr int kTomMid     = 47;
    constexpr int kTomHigh    = 50;
    constexpr int kTambourine = 54;
    constexpr int kCowbell    = 56;
    juce::ignoreUnused(kPedalHat, kCowbell); // reserved for future patterns
    int trackChannel = 1;
    if (auto* parentTrack = dynamic_cast<Track*>(midiRegion->getParentComponent()))
        trackChannel = parentTrack->getMidiChannel();
    const int kDrumChan = trackChannel;

    const double quarter   = (60.0 / bpm) * sampleRate;
    const double bar       = quarter * 4.0;
    const double eighth    = quarter / 2.0;
    const double sixteenth = quarter / 4.0;
    const double thirtysecond = quarter / 8.0;

    // Anchor: aktueller Playback-Marker, sonst 0. Snap auf das aktive Grid,
    // damit eingefuegte Patterns nicht mitten im Takt landen.
    double anchor = playbackPosition >= 0.0 ? playbackPosition : 0.0;
    if (snapToGrid) anchor = snapTimeToGrid(anchor);

    // Snapshot von neuen Notes; addNotePair mutiert die midiMessageList und
    // sortiert sie, also nicht waehrend der Schleife direkt einfuegen waere
    // unnoetiger Overhead — wir koennen aber bedenkenlos addNotePair direkt
    // rufen, weil das Pattern keine Selektion mutiert.
    auto add = [&](int pitch, double offset, double dur, int vel, int ch) {
        addNotePair(pitch, anchor + offset, dur, vel, ch);
    };

    // Hilfsfunktion: tiefste selektierte Note als Root, sonst MIDI 48 (C3).
    auto melodicRoot = [&]() {
        int lowest = 127;
        bool found = false;
        for (auto* on : selectedMessages)
            if (on != nullptr && on->isNoteOn()) {
                lowest = juce::jmin(lowest, on->getNoteNumber());
                found = true;
            }
        return found ? lowest : 48;
    };

    switch (p) {
        case Pattern::FourOnTheFloor: {
            // Kick auf jede Viertel, leichtes Velocity-Akzent auf 1.
            for (int i = 0; i < 4; ++i)
                add(kKick, i * quarter, eighth, i == 0 ? 120 : 110, kDrumChan);
            break;
        }
        case Pattern::EighthHat: {
            // 8tel-Hihat ueber einen Takt, off-beats etwas leiser.
            for (int i = 0; i < 8; ++i)
                add(kClosedHat, i * eighth, sixteenth, (i % 2 == 0) ? 90 : 70, kDrumChan);
            break;
        }
        case Pattern::SixteenthHat: {
            // 16tel-Hihat, Akzent auf jeder Viertel.
            for (int i = 0; i < 16; ++i) {
                int vel = (i % 4 == 0) ? 100 : (i % 2 == 0 ? 75 : 60);
                add(kClosedHat, i * sixteenth, thirtysecond, vel, kDrumChan);
            }
            break;
        }
        case Pattern::Backbeat: {
            // Snare auf 2 und 4 (Backbeat).
            add(kSnare, 1 * quarter, eighth, 110, kDrumChan);
            add(kSnare, 3 * quarter, eighth, 110, kDrumChan);
            break;
        }
        case Pattern::BoomBap: {
            // Kick 1 + 3, Snare 2 + 4, plus 8tel-Hihat — klassisches Hip-Hop-Schema.
            add(kKick, 0 * quarter, eighth, 120, kDrumChan);
            add(kKick, 2 * quarter, eighth, 115, kDrumChan);
            add(kSnare, 1 * quarter, eighth, 110, kDrumChan);
            add(kSnare, 3 * quarter, eighth, 110, kDrumChan);
            for (int i = 0; i < 8; ++i)
                add(kClosedHat, i * eighth, sixteenth, (i % 2 == 0) ? 80 : 60, kDrumChan);
            break;
        }
        case Pattern::HouseGroove: {
            // 4-on-the-floor Kick + Snare 2/4 + Open Hat auf den Off-Beats.
            for (int i = 0; i < 4; ++i)
                add(kKick, i * quarter, eighth, 115, kDrumChan);
            add(kSnare, 1 * quarter, eighth, 105, kDrumChan);
            add(kSnare, 3 * quarter, eighth, 105, kDrumChan);
            for (int i = 0; i < 4; ++i)
                add(kOpenHat, i * quarter + eighth, eighth, 90, kDrumChan);
            break;
        }
        case Pattern::DnBBreak: {
            // "Amen-Break"-artige Anordnung in einem Takt — angedeutet, nicht 1:1.
            add(kKick,  0 * sixteenth, eighth, 120, kDrumChan);
            add(kKick, 10 * sixteenth, eighth, 110, kDrumChan);
            add(kSnare, 4 * sixteenth, eighth, 115, kDrumChan);
            add(kSnare, 12 * sixteenth, eighth, 115, kDrumChan);
            add(kSnare, 14 * sixteenth, sixteenth, 80, kDrumChan); // Ghost-Snare
            for (int i = 0; i < 16; ++i)
                add(kClosedHat, i * sixteenth, thirtysecond, (i % 4 == 0) ? 95 : 70, kDrumChan);
            add(kRide, 0, eighth, 90, kDrumChan);
            break;
        }
        case Pattern::SnareRoll: {
            // Build-up: 32stel Snare ueber einen Takt mit linearem Velocity-Crescendo.
            const int n = 32;
            for (int i = 0; i < n; ++i) {
                const float t = (float) i / (float)(n - 1);
                const int vel = juce::jlimit(40, 127, (int) (45.0f + t * 80.0f));
                add(kSnare, i * thirtysecond, thirtysecond, vel, kDrumChan);
            }
            // Crash am Ende (= Anfang vom naechsten Takt).
            add(kCrash, bar, quarter, 120, kDrumChan);
            break;
        }
        case Pattern::OctaveBass: {
            // Quarter-Note Octave-Bouncer auf der tiefsten selektierten Note
            // (Root). Klassischer "boom-pum" Synth-Bass.
            const int root = juce::jlimit(0, 115, melodicRoot());
            for (int i = 0; i < 4; ++i) {
                const int pitch = (i % 2 == 0) ? root : (root + 12);
                add(pitch, i * quarter, eighth + sixteenth, 110, trackChannel);
            }
            break;
        }
        case Pattern::AscendingArp: {
            // 16tel-Arpeggio ueber die aktive Skala (oder chromatisch wenn leer),
            // beginnt am Root und steigt einen Takt lang.
            const int root = juce::jlimit(0, 127, melodicRoot());
            const std::vector<int>& sc = customScale;
            const bool useScale = !sc.empty();
            for (int i = 0; i < 16; ++i) {
                int pitch;
                if (useScale) {
                    const int N = (int) sc.size();
                    const int rootClass = ((root % 12) + 12) % 12;
                    int rootDeg = 0;
                    for (int k = 0; k < N; ++k) if (sc[k] == rootClass) { rootDeg = k; break; }
                    const int deg = (rootDeg + i) % N;
                    const int oct = (rootDeg + i) / N;
                    pitch = (root - rootClass) + sc[deg] + 12 * oct;
                } else {
                    pitch = root + i;
                }
                add(juce::jlimit(0, 127, pitch), i * sixteenth, sixteenth, 95, trackChannel);
            }
            break;
        }

        // ============================================================
        // Drums — Hip-Hop / Trap
        // ============================================================
        case Pattern::TrapBeat: {
            // Kick-Synkopation, Snare/Clap auf 3 (half-time-Feel), 16tel-Hat
            // mit 32stel-Roll auf der letzten Viertel — typischer Trap-Lift.
            add(kKick, 0 * quarter, eighth, 122, kDrumChan);
            add(kKick, 2 * quarter + sixteenth, eighth, 110, kDrumChan);
            add(kKick, 3 * quarter + sixteenth, eighth, 105, kDrumChan);
            add(kSnare, 2 * quarter, eighth, 115, kDrumChan);
            add(kClap,  2 * quarter, eighth, 100, kDrumChan);
            for (int i = 0; i < 12; ++i)
                add(kClosedHat, i * sixteenth, thirtysecond,
                    (i % 4 == 0) ? 95 : 70, kDrumChan);
            // 32stel-Roll im letzten Viertel — Geschwindigkeit bleibt
            // wahrnehmbar, Velocity zieht aufwaerts.
            for (int i = 0; i < 8; ++i)
                add(kClosedHat, 12 * sixteenth + i * thirtysecond,
                    thirtysecond, 75 + i * 4, kDrumChan);
            break;
        }

        // ============================================================
        // Drums — House / Techno
        // ============================================================
        case Pattern::TechnoBeat: {
            // Stoisches 4-on-floor + Snare 2/4 + 16tel Closed Hats mit
            // Akzent jeder Viertel + Open Hat auf den Off-Beat-Achteln.
            for (int i = 0; i < 4; ++i)
                add(kKick, i * quarter, eighth, 116, kDrumChan);
            add(kSnare, 1 * quarter, eighth, 105, kDrumChan);
            add(kSnare, 3 * quarter, eighth, 105, kDrumChan);
            for (int i = 0; i < 16; ++i) {
                int vel = (i % 4 == 0) ? 95 : (i % 2 == 0 ? 70 : 55);
                add(kClosedHat, i * sixteenth, thirtysecond, vel, kDrumChan);
            }
            for (int i = 0; i < 4; ++i)
                add(kOpenHat, i * quarter + eighth, sixteenth, 80, kDrumChan);
            break;
        }
        case Pattern::TechHouse: {
            // 4-floor Kick, Clap statt Snare auf 2 & 4, dichte 16tel-Hats,
            // ein paar verstreute Open-Hat-Ghosts.
            for (int i = 0; i < 4; ++i)
                add(kKick, i * quarter, eighth, 115, kDrumChan);
            add(kClap, 1 * quarter, eighth, 110, kDrumChan);
            add(kClap, 3 * quarter, eighth, 110, kDrumChan);
            for (int i = 0; i < 16; ++i)
                add(kClosedHat, i * sixteenth, thirtysecond,
                    (i % 2 == 0) ? 75 : 55, kDrumChan);
            add(kOpenHat, eighth, sixteenth, 65, kDrumChan);
            add(kOpenHat, 2 * quarter + eighth, sixteenth, 65, kDrumChan);
            break;
        }
        case Pattern::DiscoGroove: {
            // 4-floor Kick, Snare 2/4, Open Hats auf den Off-Beat-Achteln,
            // Tambourine als 16tel Shaker.
            for (int i = 0; i < 4; ++i)
                add(kKick, i * quarter, eighth, 110, kDrumChan);
            add(kSnare, 1 * quarter, eighth, 105, kDrumChan);
            add(kSnare, 3 * quarter, eighth, 105, kDrumChan);
            for (int i = 0; i < 4; ++i)
                add(kOpenHat, i * quarter + eighth, sixteenth, 90, kDrumChan);
            for (int i = 0; i < 16; ++i)
                add(kTambourine, i * sixteenth, thirtysecond, 60, kDrumChan);
            break;
        }

        // ============================================================
        // Drums — Trance / EDM
        // ============================================================
        case Pattern::TranceBeat: {
            // 4-on-floor mit klarer 8tel-Open-Hat-Linie, Clap auf 2 & 4,
            // dezenter 16tel-Closed-Hat-Layer drunter.
            for (int i = 0; i < 4; ++i)
                add(kKick, i * quarter, eighth, 118, kDrumChan);
            add(kClap, 1 * quarter, eighth, 110, kDrumChan);
            add(kClap, 3 * quarter, eighth, 110, kDrumChan);
            for (int i = 0; i < 8; ++i)
                add(kOpenHat, i * eighth + (i % 2 == 1 ? 0.0 : eighth * 0.5),
                    sixteenth, (i % 2 == 1) ? 95 : 70, kDrumChan);
            for (int i = 0; i < 16; ++i)
                add(kClosedHat, i * sixteenth, thirtysecond,
                    (i % 4 == 0) ? 75 : 50, kDrumChan);
            break;
        }

        // ============================================================
        // Drums — Drum & Bass
        // ============================================================
        case Pattern::LiquidDnB: {
            // Soulful Variante des DnB-Breaks: Kick 1 + 2.5, Snare 2/4,
            // 16tel-Hats mit Ghost-Velocity, Ride zum Schluss als Sweetener.
            add(kKick, 0 * quarter, eighth, 115, kDrumChan);
            add(kKick, 1 * quarter + eighth, eighth, 105, kDrumChan);
            add(kSnare, 1 * quarter, eighth, 110, kDrumChan);
            add(kSnare, 3 * quarter, eighth, 110, kDrumChan);
            for (int i = 0; i < 16; ++i)
                add(kClosedHat, i * sixteenth, thirtysecond,
                    (i % 4 == 0) ? 90 : 60, kDrumChan);
            add(kRide, 0, eighth, 75, kDrumChan);
            add(kRide, 2 * quarter, eighth, 75, kDrumChan);
            break;
        }

        // ============================================================
        // Drums — Dubstep
        // ============================================================
        case Pattern::DubstepHalfTime: {
            // Half-Time-Gefuehl: Kick auf 1, Snare auf 3, Hats sparsam,
            // 16tel-Roll im letzten Quarter als Drop-Vorlage.
            add(kKick, 0 * quarter, eighth, 122, kDrumChan);
            add(kSnare, 2 * quarter, quarter, 118, kDrumChan);
            add(kClosedHat, 0 * eighth, sixteenth, 80, kDrumChan);
            add(kClosedHat, 1 * eighth, sixteenth, 60, kDrumChan);
            add(kClosedHat, 4 * eighth, sixteenth, 80, kDrumChan);
            add(kClosedHat, 5 * eighth, sixteenth, 60, kDrumChan);
            for (int i = 12; i < 16; ++i)
                add(kClosedHat, i * sixteenth, thirtysecond,
                    70 + (i - 12) * 8, kDrumChan);
            break;
        }

        // ============================================================
        // Drums — Hardstyle
        // ============================================================
        case Pattern::HardstyleKick: {
            // Lange, ueberlappende Quarter-Kicks (Hardstyle/Hardcore-typische
            // Dauer-Kicks), ansteigende Velocity ueber den Takt.
            for (int i = 0; i < 4; ++i)
                add(kKick, i * quarter, quarter * 0.95, 110 + i * 3, kDrumChan);
            // Off-Beat Clap zur Anker-Strukturierung.
            add(kClap, 1 * quarter, eighth, 100, kDrumChan);
            add(kClap, 3 * quarter, eighth, 100, kDrumChan);
            break;
        }

        // ============================================================
        // Drums — World / Latin
        // ============================================================
        case Pattern::Reggaeton: {
            // Klassischer "Dem Bow"-Groove: Kick auf 1 + 3, Rim/Snare auf
            // den Off-Eighth-Counts. 8tel-Closed-Hat als Beibahnung.
            add(kKick, 0 * quarter, eighth, 115, kDrumChan);
            add(kKick, 2 * quarter, eighth, 110, kDrumChan);
            add(kRimShot, 0 * quarter + eighth, sixteenth, 100, kDrumChan);
            add(kSnare,   1 * quarter + eighth, sixteenth, 95,  kDrumChan);
            add(kRimShot, 2 * quarter + eighth, sixteenth, 100, kDrumChan);
            add(kSnare,   3 * quarter + eighth, sixteenth, 95,  kDrumChan);
            for (int i = 0; i < 8; ++i)
                add(kClosedHat, i * eighth, sixteenth, 65, kDrumChan);
            break;
        }

        // ============================================================
        // Drums — Foundational FX
        // ============================================================
        case Pattern::DrumFill: {
            // Ein-Takt-Fill: Kick auf 1, Tom-Run von tief nach hoch,
            // 16tel-Snare-Lauf auf dem letzten Viertel, Crash im naechsten Takt.
            add(kKick, 0 * quarter, eighth, 110, kDrumChan);
            add(kSnare,    1 * quarter,             sixteenth, 100, kDrumChan);
            add(kTomLow,   1 * quarter + sixteenth, sixteenth, 95,  kDrumChan);
            add(kTomLow,   1 * quarter + 2*sixteenth, sixteenth, 95, kDrumChan);
            add(kTomMid,   1 * quarter + 3*sixteenth, sixteenth, 100, kDrumChan);
            add(kTomMid,   2 * quarter,             sixteenth, 105, kDrumChan);
            add(kTomMid,   2 * quarter + sixteenth, sixteenth, 105, kDrumChan);
            add(kTomHigh,  2 * quarter + 2*sixteenth, sixteenth, 110, kDrumChan);
            add(kTomHigh,  2 * quarter + 3*sixteenth, sixteenth, 110, kDrumChan);
            for (int i = 0; i < 4; ++i)
                add(kSnare, 3 * quarter + i * sixteenth, sixteenth,
                    95 + i * 8, kDrumChan);
            add(kCrash, bar, quarter, 120, kDrumChan);
            break;
        }

        // ============================================================
        // Melodic — Bass
        // ============================================================
        case Pattern::SubBassDrone: {
            // Eine lange Note auf dem Root ueber den ganzen Takt. Druck-
            // Aufbau / Filler.
            const int root = juce::jlimit(0, 127, melodicRoot());
            add(root, 0, bar - sixteenth, 100, trackChannel);
            break;
        }
        case Pattern::AcidBassline: {
            // 303-style 16tel-Pattern: Root mit gelegentlichen Octave-Hops
            // und Velocity-Akzenten. Slides muss ein Synth dahinter loesen.
            const int root = juce::jlimit(0, 115, melodicRoot());
            static const int octShift[16] = { 0, 0,12, 0,  0,12, 0, 0,
                                              0,12, 0, 0, 12, 0, 0,12 };
            static const int velAcc[16]   = {120,60, 90,60, 100,60,80,60,
                                             110,60, 80,60,  90,60,100,60};
            for (int i = 0; i < 16; ++i) {
                const int pitch = juce::jlimit(0, 127, root + octShift[i]);
                add(pitch, i * sixteenth, sixteenth, velAcc[i], trackChannel);
            }
            break;
        }
        case Pattern::ReeseBass: {
            // DnB-Reese: Root, Quint und Octave als gestapelte ganze Note.
            // Erst der Detuning-Synth dahinter macht daraus den klassischen
            // Reese-Klang — die Notes sind nur der Trigger.
            const int root = juce::jlimit(0, 115, melodicRoot());
            add(root,      0, bar - sixteenth, 105, trackChannel);
            add(root + 7,  0, bar - sixteenth, 90,  trackChannel);
            add(root + 12, 0, bar - sixteenth, 95,  trackChannel);
            break;
        }

        // ============================================================
        // Melodic — Lead / Arp
        // ============================================================
        case Pattern::DescendingArp: {
            // Spiegel von AscendingArp; startet eine Oktave ueber dem Root
            // und faellt 16 Skalen-Schritte ab.
            const int root = juce::jlimit(0, 127, melodicRoot()) + 12;
            const std::vector<int>& sc = customScale;
            const bool useScale = !sc.empty();
            for (int i = 0; i < 16; ++i) {
                int pitch;
                if (useScale) {
                    const int N = (int) sc.size();
                    const int rootClass = ((root % 12) + 12) % 12;
                    int rootDeg = 0;
                    for (int k = 0; k < N; ++k) if (sc[k] == rootClass) { rootDeg = k; break; }
                    int idx = rootDeg - i;
                    int oct = 0;
                    while (idx < 0) { idx += N; --oct; }
                    pitch = (root - rootClass) + sc[idx] + 12 * oct;
                } else {
                    pitch = root - i;
                }
                add(juce::jlimit(0, 127, pitch), i * sixteenth, sixteenth, 95, trackChannel);
            }
            break;
        }
        case Pattern::TranceGate: {
            // Trance-Gate: 16tel-Repetition auf dem Root mit Velocity-
            // Pumpmuster wie bei einem Sidechain-Compressor / Volume-LFO.
            const int root = juce::jlimit(0, 127, melodicRoot());
            static const int gate[16] = { 127, 80,100,70, 110,60,90,50,
                                          127, 80,100,70, 110,60,90,50 };
            for (int i = 0; i < 16; ++i)
                add(root, i * sixteenth, sixteenth, gate[i], trackChannel);
            break;
        }
        case Pattern::PluckStab: {
            // Kurze Triadtoene auf jeder 8tel — Root, Quint, Octave im
            // Wechsel. Typische EDM-/Future-Bass-Pluck-Linie.
            const int root = juce::jlimit(0, 115, melodicRoot());
            for (int i = 0; i < 8; ++i) {
                const int interval = (i % 3 == 0) ? 0 : ((i % 3 == 1) ? 7 : 12);
                add(juce::jlimit(0, 127, root + interval),
                    i * eighth, sixteenth, 95, trackChannel);
            }
            break;
        }

        // ============================================================
        // Chord Progressions
        // 4 Bars, je ein Akkord pro Bar (whole note minus 16tel-Atemluft,
        // damit der Synth zwischen Akkorden re-triggert). Intervalle als
        // Halbtonschritte ueber `melodicRoot()`. Akkordfolgen in Dur klingen
        // auf einem Moll-Root entsprechend modal — der Nutzer waehlt den
        // gewuenschten Mode ueber die tiefste selektierte Note.
        // ============================================================
        case Pattern::AxisProgression:
        case Pattern::FiftiesProgression:
        case Pattern::PopSimple:
        case Pattern::SadPop:
        case Pattern::TranceEpicProgression:
        case Pattern::MinorTranceProgression:
        case Pattern::DeepHouseVamp:
        case Pattern::JazzyHouse7ths:
        case Pattern::JazzCadence:
        case Pattern::AndalusianDescent: {
            const int root = juce::jlimit(0, 127 - 24, melodicRoot());
            const double chordDur = bar - sixteenth;

            // Akkord-Tabelle pro Progression — jeweils vier Akkorde, jeder
            // Akkord ist ein Vector mit Halbton-Offsets ueber dem Root.
            std::vector<std::vector<int>> chords;
            switch (p) {
                case Pattern::AxisProgression:
                    // I - V - vi - IV  (e.g. C - G - Am - F in C major)
                    chords = { {0,4,7}, {7,11,14}, {9,12,16}, {5,9,12} };
                    break;
                case Pattern::FiftiesProgression:
                    // I - vi - IV - V (Doo-Wop)
                    chords = { {0,4,7}, {9,12,16}, {5,9,12}, {7,11,14} };
                    break;
                case Pattern::PopSimple:
                    // I - IV - V - I
                    chords = { {0,4,7}, {5,9,12}, {7,11,14}, {0,4,7} };
                    break;
                case Pattern::SadPop:
                    // vi - iii - IV - V
                    chords = { {9,12,16}, {4,7,11}, {5,9,12}, {7,11,14} };
                    break;
                case Pattern::TranceEpicProgression:
                    // vi - IV - I - V (klassischer Trance Anchor)
                    chords = { {9,12,16}, {5,9,12}, {0,4,7}, {7,11,14} };
                    break;
                case Pattern::MinorTranceProgression:
                    // i - VI - III - VII (natural minor / aeolian)
                    chords = { {0,3,7}, {8,12,15}, {3,7,10}, {10,14,17} };
                    break;
                case Pattern::DeepHouseVamp:
                    // i - iv - bVII - bIII (modal vamp)
                    chords = { {0,3,7}, {5,8,12}, {10,14,17}, {3,7,10} };
                    break;
                case Pattern::JazzyHouse7ths:
                    // iim7 - V7 - Imaj7 - Imaj7
                    chords = { {2,5,9,12}, {7,11,14,17}, {0,4,7,11}, {0,4,7,11} };
                    break;
                case Pattern::JazzCadence:
                    // ii - V - I - I  (klassische Kadenz)
                    chords = { {2,5,9}, {7,11,14}, {0,4,7}, {0,4,7} };
                    break;
                case Pattern::AndalusianDescent:
                    // i - bVII - bVI - V (Phrygian-Cadence)
                    chords = { {0,3,7}, {10,14,17}, {8,12,15}, {7,11,14} };
                    break;
                default: break;
            }

            for (size_t ci = 0; ci < chords.size(); ++ci) {
                const double offset = (double) ci * bar;
                for (int interval : chords[ci]) {
                    const int pitch = juce::jlimit(0, 127, root + interval);
                    add(pitch, offset, chordDur, 90, trackChannel);
                }
            }
            break;
        }
    }

    midiRegion->rebuildThumbnail();
    midiRegion->updateThumb();
    notifyRegionModified();
    repaint();
}

void PianoRollComponent::buildChordsFromScaleForSelection()
{
    if (customScale.empty() || selectedMessages.empty() || midiRegion == nullptr)
        return;

    // Skala ist als sortierte Tonklassen 0..11 hinterlegt (siehe
    // PianoRollEditor::applySelectedScale). Diatonischer Triad = Root +
    // dritte und fuenfte Skalenstufe ueber dem Root, mit Octave-Wrap, wenn
    // wir oben aus der Skala herauslaufen.
    const std::vector<int>& scale = customScale;
    const int N = (int) scale.size();

    // pitch des Skalenstufen-Schritts `step` ueber `rootPitch` ermitteln.
    auto pitchForStep = [&](int rootPitch, int rootDeg, int step) -> int {
        const int deg          = (rootDeg + step) % N;
        const int octaveOffset = (rootDeg + step) / N;
        const int rootClass    = ((rootPitch % 12) + 12) % 12;
        const int rootOctBase  = rootPitch - rootClass; // tiefstes C dieser Oktave
        int candidate = rootOctBase + scale[deg] + 12 * octaveOffset;
        // Sicherheitsnetz: falls der candidate doch unter dem Root landet
        // (z.B. wenn die Skala nicht streng aufsteigend ist), oktavweise
        // hochziehen bis er ueber dem Root liegt.
        while (candidate <= rootPitch) candidate += 12;
        return juce::jlimit(0, 127, candidate);
    };

    // Snapshot machen, um waehrend der Iteration nicht in die selbe Liste
    // hinein zu addieren.
    struct ChordNote { int pitch; double start; double duration; int velocity; int channel; };
    std::vector<ChordNote> toAdd;

    for (auto* on : selectedMessages) {
        if (on == nullptr || !on->isNoteOn()) continue;
        MidiMessage* off = findNoteOffForNoteOn(on);
        if (off == nullptr) continue;

        const int rootPitch = on->getNoteNumber();
        const int rootClass = ((rootPitch % 12) + 12) % 12;

        // Skalenstufe des Roots finden.
        int rootDeg = -1;
        for (int i = 0; i < N; ++i) {
            if (scale[i] == rootClass) { rootDeg = i; break; }
        }
        if (rootDeg < 0) continue; // Root nicht in Skala -> ueberspringen.

        const double start    = on->getTimeStamp();
        const double duration = off->getTimeStamp() - start;
        const int    velocity = on->getVelocity();
        const int    channel  = on->getChannel();

        toAdd.push_back({ pitchForStep(rootPitch, rootDeg, 2), start, duration, velocity, channel }); // 3rd
        toAdd.push_back({ pitchForStep(rootPitch, rootDeg, 4), start, duration, velocity, channel }); // 5th
    }

    for (const auto& c : toAdd)
        addNotePair(c.pitch, c.start, c.duration, c.velocity, c.channel);

    midiRegion->rebuildThumbnail();
    midiRegion->updateThumb();
    notifyRegionModified();
    repaint();
}

void PianoRollComponent::arpeggiate(double noteSpacing)
{
    if (selectedMessages.empty()) return;

    // Group notes that start at the same time
    std::map<int, std::vector<MidiMessage*>> timeGroups;
    for (auto* msg : selectedMessages) {
        int startSample = (int)msg->getTimeStamp();
        timeGroups[startSample].push_back(msg);
    }

    for (auto& [startSample, notes] : timeGroups) {
        if (notes.size() <= 1) continue;

        // Sort by pitch
        std::sort(notes.begin(), notes.end(), [](MidiMessage* a, MidiMessage* b) {
            return a->getNoteNumber() < b->getNoteNumber();
            });

        // Spread notes over time
        for (size_t i = 0; i < notes.size(); ++i) {
            MidiMessage* noteOff = findNoteOffForNoteOn(notes[i]);
            if (!noteOff) continue;

            double originalStart = notes[i]->getTimeStamp();
            double originalEnd = noteOff->getTimeStamp();
            double duration = originalEnd - originalStart;
            double offset = i * noteSpacing * sampleRate;

            notes[i]->setTimeStamp(originalStart + offset);
            noteOff->setTimeStamp(originalStart + offset + duration);
        }
    }

    midiRegion->rebuildThumbnail();
    midiRegion->updateThumb();
    notifyRegionModified();
    repaint();
}

// =================== CONTROLLER AUTOMATION ===================

void PianoRollComponent::addControllerLane(int ccNumber, const juce::String& name, juce::Colour colour)
{
    ControllerLane lane;
    lane.ccNumber = ccNumber;
    lane.name = name;
    lane.colour = colour;
    lane.visible = true;

    controllerLanes[ccNumber] = lane;

    // Adjust component height
    int totalLanes = 0;
    for (const auto& [cc, laneData] : controllerLanes) {
        if (laneData.visible) totalLanes++;
    }

    setSize(getWidth(), totalHeight + totalLanes * controllerLaneHeight);
    repaint();
}

void PianoRollComponent::setControllerLaneVisibility(int ccNumber, bool visible)
{
    if (controllerLanes.find(ccNumber) != controllerLanes.end()) {
        controllerLanes[ccNumber].visible = visible;

        // Adjust component height
        int totalLanes = 0;
        for (const auto& [cc, laneData] : controllerLanes) {
            if (laneData.visible) totalLanes++;
        }

        setSize(getWidth(), totalHeight + totalLanes * controllerLaneHeight);
        repaint();
    }
}

// =================== PLAYBACK INTEGRATION ===================

void PianoRollComponent::setPlaybackPosition(double samplePosition)
{
    // Caller (TrackNavigator-Timer) uebergibt eine TIMELINE-relative
    // Sample-Position. getXForTime() interpretiert seinen Eingang aber als
    // BUFFER-relativ (so wie midiMessageList-Timestamps), darum hier
    // einmalig konvertieren:
    //   buffer = (timeline - sampleOffset) + sourceStartSample
    // Ohne diese Korrektur driftet der Playhead in der Piano Roll um
    // sampleOffset Pixel ab, sobald die Region nicht bei Timeline 0 sitzt
    // (und um zusaetzlich sourceStartSample, falls links getrimmt).
    const double offsetSample = (midiRegion != nullptr) ? (double) midiRegion->getSampleOffset() : 0.0;
    const double base         = (midiRegion != nullptr) ? (double) midiRegion->getSourceStartSample() : 0.0;
    playbackPosition = samplePosition - offsetSample + base;

    // Auto-follow playback
    if (followPlayback && playbackPosition >= 0.0) {
        int playheadX = getXForTime(playbackPosition);
        int viewportCenter = getWidth() / 2;

        if (playheadX < 50 || playheadX > getWidth() - 50) {
            // Adjust pan to keep playhead visible
            panOffset.x = viewportCenter - getXForTime(playbackPosition) + pianoWidth;
            panOffset.x = juce::jmin(panOffset.x, 0.0f);
            repaint();
        }
    }
}

void PianoRollComponent::setLoopRegion(double startSample, double endSample)
{
    hasLoopRegion = true;
    loopStart = startSample;
    loopEnd = endSample;
    repaint();
}

// =================== SETTINGS GETTERS ===================

void PianoRollComponent::setGridResolution(int denominator)
{
    // gridResolution wird im Rest des Codes als "Bruchteil einer Viertelnote"
    // verwendet (siehe paint, getGridTimeStep, snapTimeToGrid). Eine 1/N-Note
    // entspricht 4/N einer Viertelnote — z.B. 1/16-Note = 4/16 einer Viertel.
    // Vorher stand hier 1.0f / denominator, was bei setGridResolution(16) den
    // Grid-Step auf Viertel/16 = 1/64-Note gesetzt hat: Pfeiltasten haben
    // dann pro Druck nur 1/64 verschoben und es sah aus, als wuerde die Note
    // zufaellig springen.
    if (denominator <= 0) return;
    gridResolution = 4.0f / denominator;
    repaint();
}

void PianoRollComponent::resized()
{
    // Update any child components here if needed
}

// =================== COMMAND TARGET IMPLEMENTATION ===================

void PianoRollComponent::getAllCommands(juce::Array<juce::CommandID>& commands)
{
    commands.add(selectAllNotes);
    commands.add(selectNo);
    commands.add(copy);
    commands.add(cut);
    commands.add(paste);
    commands.add(deleteSelected);
    commands.add(undo);
    commands.add(redo);
    commands.add(quantize);
    commands.add(humanize);
    commands.add(transposeUp);
    commands.add(transposeDown);
    commands.add(velocityIncrease);
    commands.add(velocityDecrease);
    commands.add(duplicateNotes);
    commands.add(reverseNotes);
}

void PianoRollComponent::getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result)
{
    switch (commandID) {
    case selectAllNotes:
        result.setInfo("Select All", "Select all notes", "Edit", 0);
        result.defaultKeypresses.add(juce::KeyPress('a', juce::ModifierKeys::ctrlModifier, 0));
        break;

    case selectNo:
        result.setInfo("Select None", "Clear selection", "Edit", 0);
        result.defaultKeypresses.add(juce::KeyPress('d', juce::ModifierKeys::ctrlModifier, 0));
        break;

    case copy:
        result.setInfo("Copy", "Copy selected notes", "Edit", 0);
        result.defaultKeypresses.add(juce::KeyPress('c', juce::ModifierKeys::ctrlModifier, 0));
        result.setActive(!selectedMessages.empty());
        break;

    case cut:
        result.setInfo("Cut", "Cut selected notes", "Edit", 0);
        result.defaultKeypresses.add(juce::KeyPress('x', juce::ModifierKeys::ctrlModifier, 0));
        result.setActive(!selectedMessages.empty());
        break;

    case paste:
        result.setInfo("Paste", "Paste notes", "Edit", 0);
        result.defaultKeypresses.add(juce::KeyPress('v', juce::ModifierKeys::ctrlModifier, 0));
        result.setActive(canPaste());
        break;

    case deleteSelected:
        result.setInfo("Delete", "Delete selected notes", "Edit", 0);
        result.defaultKeypresses.add(juce::KeyPress(juce::KeyPress::deleteKey, 0, 0));
        result.setActive(!selectedMessages.empty());
        break;

    case quantize:
        result.setInfo("Quantize", "Quantize selected notes to grid", "Edit", 0);
        result.defaultKeypresses.add(juce::KeyPress('q', juce::ModifierKeys::ctrlModifier, 0));
        result.setActive(!selectedMessages.empty());
        break;

    case transposeUp:
        result.setInfo("Transpose Up", "Transpose selected notes up one semitone", "Edit", 0);
        result.defaultKeypresses.add(juce::KeyPress(juce::KeyPress::upKey, 0, 0));
        result.setActive(!selectedMessages.empty());
        break;

    case transposeDown:
        result.setInfo("Transpose Down", "Transpose selected notes down one semitone", "Edit", 0);
        result.defaultKeypresses.add(juce::KeyPress(juce::KeyPress::downKey, 0, 0));
        result.setActive(!selectedMessages.empty());
        break;
    }
}

bool PianoRollComponent::perform(const InvocationInfo& info)
{ 
    switch (info.commandID) {
    case selectAllNotes:    selectAll(); return true;
    case selectNo:          selectNone(); return true;
    case copy:              copySelected(); return true;
    case cut:               cutSelected(); return true;
    case paste:             pasteAtPosition(playbackPosition >= 0 ? playbackPosition : 0); return true;
    case deleteSelected:    cutSelected(); return true;
    case quantize:          quantizeSelected(); return true;
    case humanize:          humanizeSelected(); return true;
    case transposeUp:       transposeSelected(1); return true;
    case transposeDown:     transposeSelected(-1); return true;
    case velocityIncrease:  scaleSelectedVelocity(1.1f); return true;
    case velocityDecrease:  scaleSelectedVelocity(0.9f); return true;
    case duplicateNotes:    duplicateSelected(); return true;
    case reverseNotes:      reverseSelected(); return true;
    default:                return false;
    }
}

// =================== FINAL HELPER METHODS ===================

void PianoRollComponent::moveNote(MidiMessage* noteOn, double deltaTime, int deltaPitch)
{
    MidiMessage* noteOff = findNoteOffForNoteOn(noteOn);
    if (!noteOff) return;

    // Move temporally
    noteOn->setTimeStamp(juce::jmax(0.0, noteOn->getTimeStamp() + deltaTime));
    noteOff->setTimeStamp(juce::jmax(noteOn->getTimeStamp() + 10.0, noteOff->getTimeStamp() + deltaTime));

    // Change pitch
    int newPitch = juce::jlimit(0, 127, noteOn->getNoteNumber() + deltaPitch);
    noteOn->setNoteNumber(newPitch);
    noteOff->setNoteNumber(newPitch);
}

void PianoRollComponent::resizeNote(MidiMessage* noteOn, double newDuration)
{
    MidiMessage* noteOff = findNoteOffForNoteOn(noteOn);
    if (!noteOff) return;

    noteOff->setTimeStamp(noteOn->getTimeStamp() + juce::jmax(10.0, newDuration));
}

void PianoRollComponent::setNoteVelocity(MidiMessage* noteOn, int velocity)
{
    int oldVelocity = noteOn->getVelocity();
    noteOn->setVelocity(juce::jlimit(1, 127, velocity));

    if (onVelocityChanged) {
        onVelocityChanged(noteOn, oldVelocity, velocity);
    }
}

bool PianoRollComponent::isOverVelocityHandle(juce::Point<int> position, MidiMessage* note)
{
    if (!showVelocityAsHeight) return false;

    auto rect = getNoteRect(*note, getNoteDuration(note));
    return rect.contains(position) && position.y > rect.getBottom() - 8;
}
void PianoRollComponent::setCurrentTool(Tool newTool)
{
    if (currentTool != newTool) {
        currentTool = newTool;
        setMouseCursor(getCursorForTool(currentTool));
        repaint();
        if (onToolChanged) onToolChanged(currentTool);
    }
}

// 7. Improve cursor handling in mouseMove
void PianoRollComponent::mouseMove(const juce::MouseEvent& e)
{
    if (currentTool == Tool::SELECT) {
        MidiMessage* hitNote = getNoteAt(e.getPosition());

        if (hitNote) {
            auto rect = getNoteRect(*hitNote, getNoteDuration(hitNote));
            bool isOverResizeHandle = (e.x >= rect.getRight() - 8) && (e.x <= rect.getRight() + 4);

            if (isOverResizeHandle) {
                setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
            }
            else if (isOverVelocityHandle(e.getPosition(), hitNote)) {
                setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
            }
            else {
                setMouseCursor(juce::MouseCursor::PointingHandCursor);
            }
        }
        else {
            setMouseCursor(getCursorForTool(currentTool));
        }
    }
}

