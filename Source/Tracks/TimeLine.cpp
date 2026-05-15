/*
  ==============================================================================
    TimeLine.cpp
    Created: 25 Nov 2016 10:54:33am
    Author:  pueskma
    Extended: Loop functionality between two locators with 1/4 beat snap
  ==============================================================================
*/
#include "../JuceLibraryCode/JuceHeader.h"
#include "TimeLine.h"
#include <sstream>
#include <iostream>
#include "../TransportDisplayComponent.h"
#include "../MessageBus/FastBus.h"
#include "../AudioManager.h"

//==============================================================================
TimeLine::TimeLine(double lengthInSeconds)
{
    this->length = lengthInSeconds;
    dropShadow = std::make_unique<DropShadow>(Colour::fromFloatRGBA(0, 0, 0, 0.5), 3, Point<int>(3, 3));
    dropShadower = std::make_unique<DropShadower>(*dropShadow);
    dropShadower->setOwner(this);

    auto& bus = FastBus::getInstance();

    bus.subscribe<float>("tempo", [this](const FastTopic<float>& topic) {
        bpm = topic.getValue();
        repaint();
    });

    // Reservieren, damit add/remove waehrend laufendem Transport keine
    // Reallocation triggern — der Audio-Thread iteriert ueber `markers` ohne
    // Lock und wuerde sonst auf freigegebenen Speicher zugreifen. 64 Marker
    // ist mehr als jedes realistische Projekt jemals haben wird.
    markers.reserve(64);
}

TimeLine::~TimeLine() {}

void TimeLine::paint(Graphics& g)
{
    g.fillAll(Colours::darkgrey);
    g.setColour(Colours::black);
    g.drawRect(getLocalBounds(), 1);
    g.setColour(Colours::white);

    if (length > 0 && bpm > 0.0 && timeSigDenominator > 0)
    {
        const double secondsPerBeat = 60.0 / bpm;
        const double beatsPerBar = static_cast<double>(timeSigNumerator);
        const double secondsPerBar = secondsPerBeat * beatsPerBar;
        // pixelsPerSecond muss exakt dem Track-Zoom entsprechen, sonst
        // läuft das Raster gegenüber den Regions auseinander.
        // Fallback: aus Breite/Length ableiten (z. B. wenn zoom noch nicht gesetzt).
        const double pixelsPerSecond = (zoom > 0.0f) ? static_cast<double>(zoom)
                                                     : (getWidth() / length);
        // Anzahl Takte aus dem tatsächlich sichtbaren Pixelbereich ableiten,
        // damit auch dann genug Bars gezeichnet werden, wenn die TimeLine
        // breiter ist als `length` (Tracks können länger werden als
        // Project::getTrackLength()).
        const double visibleSeconds = (pixelsPerSecond > 0.0)
            ? static_cast<double>(getWidth()) / pixelsPerSecond
            : length;
        const int totalBars = static_cast<int>(visibleSeconds / secondsPerBar) + 1;

        // Bei niedrigem Zoom werden Takte/Beats sonst zu eng beieinander
        // gezeichnet. Ab Zoomstufe <= 10 nur noch jeden 4. Takt anzeigen
        // (ohne Beat-/Viertel-Unterteilungen).
        const bool sparseMode = (zoom <= 10.0f);
        const int barStep = sparseMode ? 4 : 1;

        // Haupt-Taktlinien und Beat-Linien zeichnen
        for (int bar = 1; bar <= totalBars; ++bar)
        {
            const bool drawThisBar = ((bar - 1) % barStep == 0);
            if (!drawThisBar)
                continue;

            const double barStartSec = (bar - 1) * secondsPerBar;
            const int beatsToDraw = sparseMode ? 1 : timeSigNumerator;

            for (int beat = 0; beat < beatsToDraw; ++beat)
            {
                const double beatTimeSec = barStartSec + beat * secondsPerBeat;
                const float x = static_cast<float>(beatTimeSec * pixelsPerSecond + offset);

                // L�ngere Linie am Taktanfang
                const bool isBarStart = (beat == 0);
                float lineHeight = isBarStart ? getHeight() / 2.0f : getHeight() / 3.0f;

                // Linie bei "Takt 1" (= Beschriftung "1" mit 0-basiertem Label,
                // d.h. bar==2 in der 1-basierten Schleife) deutlich dicker —
                // markiert visuell den Beginn des ersten gespielten Taktes.
                const bool isTaktOne = (isBarStart && bar == 2);
                if (isTaktOne)
                {
                    g.drawLine(x, 0.0f, x, getHeight() * 0.85f, 3.0f);
                }
                else
                {
                    g.drawLine(x, 0.0f, x, lineHeight);
                }

                // 1/4-Takt Unterteilungen zeichnen (schw�chere Linien)
                if (!sparseMode)
                {
                    g.setColour(Colours::lightgrey);
                    for (int quarter = 1; quarter < 4; ++quarter)
                    {
                        const double quarterTimeSec = beatTimeSec + (quarter * secondsPerBeat / 4.0);
                        const float quarterX = static_cast<float>(quarterTimeSec * pixelsPerSecond + offset);
                        if (quarterX < getWidth())
                        {
                            g.drawLine(quarterX, 0.0f, quarterX, getHeight() / 6.0f);
                        }
                    }
                    g.setColour(Colours::white);
                }

                // Nur Bar-Zahlen mittig zeichnen. Beschriftung beginnt bei 0
                // (am Timeline-Anfang) — bar laeuft 1..totalBars, also -1.
                if (isBarStart)
                {
                    String label = String(bar - 1);
                    const int textWidth = 30;
                    const int textHeight = 12;
                    g.drawText(label,
                        static_cast<int>(x - textWidth / 2),
                        getHeight() - textHeight - 2,
                        textWidth, textHeight,
                        juce::Justification::centred);
                }
            }
        }

        // Marker (Wavelab-style) — werden ueber der Bar/Beat-Grid gezeichnet,
        // aber unter dem Loop-Locator-Overlay, damit die Locatoren weiter klar
        // erkennbar bleiben wenn sich Positionen ueberlappen.
        for (size_t i = 0; i < markers.size(); ++i)
        {
            const auto& m = markers[i];
            const float x = timeToPixel(m.position);
            if (x < -20.0f || x > getWidth() + 20.0f) continue;

            const bool isLoop = (m.type == TimelineMarker::Type::Loop);
            const juce::Colour col = isLoop ? juce::Colour(0xfff5a623)   // orange — Loop
                                            : juce::Colour(0xfff5e23a); // gelb   — Normal

            // Vertikale Linie ueber gesamte Hoehe
            g.setColour(col.withAlpha(0.85f));
            g.drawLine(x, 0.0f, x, (float) getHeight(), 1.5f);

            // Fahnen-Dreieck nach rechts, oben
            juce::Path flag;
            flag.addTriangle(x,         0.0f,
                             x + 10.0f, 4.0f,
                             x,         8.0f);
            g.setColour(col);
            g.fillPath(flag);

            // Label rechts neben der Fahne — Name plus "xN" bei Loop-Markern
            juce::String label = m.name;
            if (isLoop) {
                if (label.isNotEmpty()) label << " ";
                label << "x" << m.loopRepeats;
            }
            if (label.isNotEmpty()) {
                g.setColour(juce::Colours::white);
                g.setFont(11.0f);
                g.drawText(label,
                           (int) (x + 12), 0,
                           120, 12,
                           juce::Justification::centredLeft, true);
            }
        }

        // Loop-Region zeichnen
        if (loopEnabled && loopEnd > loopStart)
        {
            float startX = timeToPixel(loopStart);
            float endX = timeToPixel(loopEnd);

            // Loop-Region mit transparenter Farbe hervorheben
            if (endX > startX) {
                g.setColour(Colour::fromFloatRGBA(0.0f, 1.0f, 0.0f, 0.2f));
                g.fillRect(startX, 0.0f, endX - startX, static_cast<float>(getHeight()));
            }

            // Loop-Locatoren zeichnen
            g.setColour(Colours::green);
            const float locatorWidth = 2.0f;

            // Start-Locator (Dreieck nach rechts)
            Path startTriangle;
            startTriangle.addTriangle(startX, 0.0f,
                startX + 8.0f, 4.0f,
                startX, 8.0f);
            g.fillPath(startTriangle);
            g.drawLine(startX, 0.0f, startX, static_cast<float>(getHeight()), locatorWidth);

            // End-Locator (Dreieck nach links)  
            Path endTriangle;
            endTriangle.addTriangle(endX, 0.0f,
                endX - 8.0f, 4.0f,
                endX, 8.0f);
            g.fillPath(endTriangle);
            g.drawLine(endX, 0.0f, endX, static_cast<float>(getHeight()), locatorWidth);

            // Loop-Beschriftung – nur zeichnen wenn genug Platz vorhanden
            float labelWidth = endX - startX - 20.0f;
            if (labelWidth > 0.0f) {
                g.setColour(Colours::white);
                String loopText = "LOOP: " + secondsToTime(static_cast<int>(loopStart)) +
                    " - " + secondsToTime(static_cast<int>(loopEnd));
                g.drawText(loopText,
                    static_cast<int>(startX + 10), 2,
                    static_cast<int>(labelWidth), 12,
                    juce::Justification::left);
            }
        }
    }
}

void TimeLine::mouseMove(const MouseEvent& event)
{
    if (!loopEnabled)
    {
        setMouseCursor(MouseCursor::NormalCursor);
        return;
    }

    const float tolerance = 5.0f; // Pixel-Toleranz f�r Hover-Bereich

    // Pr�fen ob Maus �ber Loop-Start oder Loop-End Locator ist
    if (isMouseOverLocator(event, loopStart, tolerance) ||
        isMouseOverLocator(event, loopEnd, tolerance))
    {
        setMouseCursor(MouseCursor::LeftRightResizeCursor);
    }
    else if (event.getPosition().x >= timeToPixel(loopStart) &&
        event.getPosition().x <= timeToPixel(loopEnd))
    {
        // Innerhalb der Loop-Region - normaler Cursor oder Bewegungs-Cursor
        setMouseCursor(MouseCursor::DraggingHandCursor);
    }
    else
    {
        setMouseCursor(MouseCursor::NormalCursor);
    }
}

void TimeLine::mouseDown(const MouseEvent& event)
{
    // Mittlere Maustaste ist fuers Pannen reserviert.
    if (event.mods.isMiddleButtonDown())
        return;

    // Right-Click: Marker-Kontextmenue. Liegt der Klick auf einem Marker, dann
    // Marker-Menue; sonst "Add Marker here" am Klickpunkt. Wir verlassen die
    // Funktion danach — kein Loop-Drag triggern.
    if (event.mods.isRightButtonDown())
    {
        const int idx = markerIndexAt(event);
        const juce::Point<int> screen = event.getScreenPosition();
        if (idx >= 0)
            showMarkerContextMenu(idx, screen);
        else {
            const double t = juce::jmax(0.0,
                snapToBufferBoundary(snapToQuarterBeat(pixelToTime((float) event.getMouseDownX()))));
            showEmptyContextMenu(t, screen);
        }
        return;
    }

    // Left-Click auf einen Marker -> Drag-to-move. Hat Vorrang vor Loop-Drag,
    // weil Marker auch ausserhalb der Loop-Region existieren koennen.
    {
        const int idx = markerIndexAt(event);
        if (idx >= 0)
        {
            currentDragMode = DragMarker;
            draggingMarker  = idx;
            dragStartOffset = pixelToTime((float) event.getMouseDownX()) - markers[(size_t) idx].position;
            return;
        }
    }

    if (!loopEnabled)
        return;

    double clickTime = snapToBufferBoundary(snapToQuarterBeat(pixelToTime(static_cast<float>(event.getMouseDownX()))));

    // Pr�fen ob auf einen Locator geklickt wurde
    if (isMouseOverLocator(event, loopStart))
    {
        currentDragMode = DragLoopStart;
        dragStartOffset = pixelToTime(static_cast<float>(event.getMouseDownX())) - loopStart;
    }
    else if (isMouseOverLocator(event, loopEnd))
    {
        currentDragMode = DragLoopEnd;
        dragStartOffset = pixelToTime(static_cast<float>(event.getMouseDownX())) - loopEnd;
    }
    else if (clickTime >= loopStart && clickTime <= loopEnd)
    {
        currentDragMode = DragLoopRegion;
        dragStartOffset = pixelToTime(static_cast<float>(event.getMouseDownX())) - loopStart;
    }
    else
    {
        // Neue Loop-Region beginnen
        currentDragMode = DragLoopEnd;
        loopStart = clickTime;
        loopEnd = clickTime;
        dragStartOffset = 0.0;
        repaint();
    }
}

void TimeLine::mouseDrag(const MouseEvent& event)
{
    if (currentDragMode == None)
        return;

    double currentTime = pixelToTime(static_cast<float>(event.getPosition().x));
    currentTime = juce::jmax(0.0, juce::jmin(length, currentTime)); // Auf Timeline begrenzen

    switch (currentDragMode)
    {
    case DragMarker:
    {
        if (draggingMarker < 0 || draggingMarker >= (int) markers.size())
            break;
        double newPos = snapToBufferBoundary(snapToQuarterBeat(currentTime - dragStartOffset));
        newPos = juce::jmax(0.0, juce::jmin(length, newPos));
        markers[(size_t) draggingMarker].position = newPos;
        // Liste sortiert halten — vereinfacht die "previous marker"-Suche im
        // Player. Wir merken uns die alte Identitaet ueber Name+Type+Repeats
        // ist hier nicht noetig, weil der Index nach dem sort wieder neu
        // bestimmt wird beim naechsten mouseDown.
        std::sort(markers.begin(), markers.end(),
                  [](const TimelineMarker& a, const TimelineMarker& b){
                      return a.position < b.position;
                  });
        // Nach dem Sort kann sich der Index verschoben haben.
        for (size_t i = 0; i < markers.size(); ++i) {
            if (std::abs(markers[i].position - newPos) < 1.0e-9) {
                draggingMarker = (int) i;
                break;
            }
        }
        break;
    }
    case DragLoopStart:
    {
        double newStart = snapToBufferBoundary(snapToQuarterBeat(currentTime - dragStartOffset));
        newStart = juce::jmax(0.0, juce::jmin(loopEnd - getMinLoopDuration(), newStart));
        loopStart = newStart;
        break;
    }
    case DragLoopEnd:
    {
        double newEnd = snapToBufferBoundary(snapToQuarterBeat(currentTime - dragStartOffset));
        newEnd = juce::jmax(loopStart + getMinLoopDuration(), juce::jmin(length, newEnd));
        loopEnd = newEnd;
        break;
    }
    case DragLoopRegion:
    {
        double loopLength = loopEnd - loopStart;
        double newStart = snapToBufferBoundary(snapToQuarterBeat(currentTime - dragStartOffset));
        newStart = juce::jmax(0.0, juce::jmin(length - loopLength, newStart));
        loopStart = newStart;
        loopEnd = newStart + loopLength;
        break;
    }
    default:
        break;
    }

    repaint();
}

void TimeLine::mouseUp(const MouseEvent& event)
{
    if (currentDragMode == DragMarker) {
        notifyChanged();
    }
    else if (currentDragMode != None && onLoopChanged)
    {
        onLoopChanged(loopStart, loopEnd, loopEnabled);
    }
    currentDragMode = None;
    draggingMarker  = -1;
}

void TimeLine::setLength(double length)
{
    this->length = length;
    // Loop-Region anpassen falls sie au�erhalb der neuen L�nge liegt
    if (loopEnd > length)
    {
        loopEnd = length;
        if (loopStart >= loopEnd)
        {
            loopStart = juce::jmax(0.0, loopEnd - getMinLoopDuration());
        }
    }
    repaint();
}

void TimeLine::resized() {}

void TimeLine::changeListenerCallback(ChangeBroadcaster* source) {
}

void TimeLine::setTempoAndTimeSignature(double bpm_, int numerator, int denominator)
{
    bpm = bpm_;
    timeSigNumerator = numerator;
    timeSigDenominator = denominator;
    repaint();
}

void TimeLine::setZoom(float zoom_)
{
    zoom = zoom_;
    repaint();
}

String TimeLine::secondsToTime(int seconds)
{
    int min = (seconds % 3600) / 60;
    std::stringstream out;
    out << min << ":";
    int sec = (seconds % 3600) % 60;
    if (sec < 10) {
        out << "0";
    }
    out << sec;
    return String(out.str());
}

void TimeLine::setOffset(int offset)
{
    this->offset = offset;
    repaint();
}

// Loop-Funktionen
void TimeLine::setLoopEnabled(bool enabled)
{
    loopEnabled = enabled;
    repaint();
}

void TimeLine::setLoopStart(double startTime)
{
    loopStart = snapToBufferBoundary(snapToQuarterBeat(juce::jmax(0.0, juce::jmin(length, startTime))));
    if (loopStart >= loopEnd)
    {
        loopEnd = snapToBufferBoundary(snapToQuarterBeat(juce::jmin(length, loopStart + getMinLoopDuration())));
    }
    repaint();
}

void TimeLine::setLoopEnd(double endTime)
{
    loopEnd = snapToBufferBoundary(snapToQuarterBeat(juce::jmax(0.0, juce::jmin(length, endTime))));
    if (loopEnd <= loopStart)
    {
        loopStart = snapToBufferBoundary(snapToQuarterBeat(juce::jmax(0.0, loopEnd - getMinLoopDuration())));
    }
    repaint();
}

void TimeLine::setLoopRegion(double startTime, double endTime)
{
    if (startTime > endTime)
        std::swap(startTime, endTime);

    loopStart = snapToBufferBoundary(snapToQuarterBeat(juce::jmax(0.0, juce::jmin(length, startTime))));
    loopEnd = snapToBufferBoundary(snapToQuarterBeat(juce::jmax(loopStart + getMinLoopDuration(), juce::jmin(length, endTime))));
    repaint();

    if (onLoopChanged)
    {
        onLoopChanged(loopStart, loopEnd, loopEnabled);
    }
}

void TimeLine::clearLoop()
{
    loopStart = 0.0;
    loopEnd = 0.0;
    repaint();

    if (onLoopChanged)
    {
        onLoopChanged(loopStart, loopEnd, loopEnabled);
    }
}

// Helper-Funktionen
double TimeLine::pixelToTime(float pixel) const
{
    if (zoom > 0.0f)
        return (pixel - offset) / static_cast<double>(zoom);

    if (length <= 0 || getWidth() <= 0)
        return 0.0;

    return ((pixel - offset) / getWidth()) * length;
}

float TimeLine::timeToPixel(double time) const
{
    if (zoom > 0.0f)
        return static_cast<float>(time * zoom + offset);

    if (length <= 0)
        return 0.0f;

    return static_cast<float>((time / length) * getWidth() + offset);
}

bool TimeLine::isMouseOverLocator(const MouseEvent& event, double locatorTime, float tolerance) const
{
    float locatorX = timeToPixel(locatorTime);
    return std::abs(event.getPosition().x - locatorX) <= tolerance;
}

// Snap-Funktionen
double TimeLine::snapToQuarterBeat(double time) const
{
    if (bpm <= 0.0)
        return time;

    const double quarterBeatDuration = getQuarterBeatDuration();
    if (quarterBeatDuration <= 0.0)
        return time;

    // Auf n�chstes 1/4-Beat runden
    const double quarterBeats = time / quarterBeatDuration;
    const double roundedQuarterBeats = std::round(quarterBeats);

    return roundedQuarterBeats * quarterBeatDuration;
}

double TimeLine::getQuarterBeatDuration() const
{
    if (bpm <= 0.0)
        return 0.0;

    const double secondsPerBeat = 60.0 / bpm;
    return secondsPerBeat / 4.0; // Ein Viertel eines Beats
}

// Buffer size alignment functions
double TimeLine::snapToBufferBoundary(double time) const
{
    double bufferDuration = getBufferDurationInSeconds();
    if (bufferDuration <= 0.0)
        return time;

    // Round to nearest buffer boundary
    const double buffers = time / bufferDuration;
    const double roundedBuffers = std::round(buffers);

    return roundedBuffers * bufferDuration;
}

double TimeLine::getBufferDurationInSeconds() const
{
    int bufferSize = getCurrentBufferSize();
    double sampleRate = getCurrentSampleRate();
    
    if (bufferSize <= 0 || sampleRate <= 0.0)
        return 0.0;

    return static_cast<double>(bufferSize) / sampleRate;
}

int TimeLine::getCurrentBufferSize() const
{
    // Try to get buffer size from AudioManager
    auto* audioManager = AudioManager::getInstance();
    if (audioManager && audioManager->getDeviceManager())
    {
        auto* device = audioManager->getDeviceManager()->getCurrentAudioDevice();
        if (device)
        {
            return device->getCurrentBufferSizeSamples();
        }
    }
    
    // Fallback to common buffer size if AudioManager is not available
    return 512;
}

double TimeLine::getCurrentSampleRate() const
{
    // Try to get sample rate from AudioManager
    auto* audioManager = AudioManager::getInstance();
    if (audioManager && audioManager->getDeviceManager())
    {
        auto* device = audioManager->getDeviceManager()->getCurrentAudioDevice();
        if (device)
        {
            return device->getCurrentSampleRate();
        }
    }
    
    // Fallback to common sample rate if AudioManager is not available
    return 44100.0;
}

double TimeLine::getMinLoopDuration() const
{
    // Ensure minimum loop duration is at least one buffer size
    double bufferDuration = getBufferDurationInSeconds();
    double quarterBeatDuration = getQuarterBeatDuration();

    // Use the larger of buffer duration or quarter beat duration
    return juce::jmax(bufferDuration, quarterBeatDuration);
}

// ==================== Marker-Management ====================

void TimeLine::addMarker(const TimelineMarker& m)
{
    markers.push_back(m);
    std::sort(markers.begin(), markers.end(),
              [](const TimelineMarker& a, const TimelineMarker& b){
                  return a.position < b.position;
              });
    repaint();
    notifyChanged();
}

void TimeLine::removeMarker(int index)
{
    if (index < 0 || index >= (int) markers.size()) return;
    markers.erase(markers.begin() + index);
    repaint();
    notifyChanged();
}

void TimeLine::clearMarkers()
{
    markers.clear();
    repaint();
    notifyChanged();
}

void TimeLine::resetLoopCounters()
{
    for (auto& m : markers)
        m.repeatsLeft = (m.type == TimelineMarker::Type::Loop) ? m.loopRepeats : 0;
}

void TimeLine::notifyChanged()
{
    if (onMarkersChanged) onMarkersChanged();
}

int TimeLine::markerIndexAt(const juce::MouseEvent& event, float tolerance) const
{
    const float mx = (float) event.getPosition().x;
    int   best     = -1;
    float bestDist = tolerance;
    for (size_t i = 0; i < markers.size(); ++i) {
        const float mxMarker = timeToPixel(markers[i].position);
        const float d = std::abs(mx - mxMarker);
        if (d <= bestDist) { bestDist = d; best = (int) i; }
    }
    return best;
}

void TimeLine::showEmptyContextMenu(double timeAtClick, juce::Point<int> /*screenPos*/)
{
    juce::PopupMenu m;
    m.addItem(1, "Add Marker here");
    m.addItem(2, "Add Loop Marker here");

    // captureSafe: timeAtClick by value, this per pointer. PopupMenu wird async
    // ausgewertet — auf den TimeLine-Pointer ist via SafePointer abzusichern,
    // damit ein Tab-Wechsel zwischen Klick und Auswahl kein Crash gibt.
    juce::Component::SafePointer<TimeLine> safe(this);
    m.showMenuAsync(juce::PopupMenu::Options(),
        [safe, timeAtClick](int result) {
            if (safe == nullptr || result == 0) return;
            TimelineMarker nm;
            nm.position    = timeAtClick;
            nm.name        = "Marker";
            nm.type        = (result == 2) ? TimelineMarker::Type::Loop
                                           : TimelineMarker::Type::Normal;
            nm.loopRepeats = 1;
            safe->addMarker(nm);
        });
}

void TimeLine::showMarkerContextMenu(int markerIndex, juce::Point<int> /*screenPos*/)
{
    if (markerIndex < 0 || markerIndex >= (int) markers.size()) return;
    const auto& m = markers[(size_t) markerIndex];

    juce::PopupMenu menu;
    menu.addSectionHeader(m.name.isEmpty() ? juce::String("Marker") : m.name);
    menu.addItem(1, "Rename...");
    menu.addSeparator();

    juce::PopupMenu typeMenu;
    typeMenu.addItem(10, "Normal", true, m.type == TimelineMarker::Type::Normal);
    typeMenu.addItem(11, "Loop",   true, m.type == TimelineMarker::Type::Loop);
    menu.addSubMenu("Type", typeMenu);

    juce::PopupMenu repMenu;
    const int repeatChoices[] = { 1, 2, 3, 4, 8, 16 };
    for (int i = 0; i < 6; ++i) {
        repMenu.addItem(100 + repeatChoices[i],
                        "Repeat " + juce::String(repeatChoices[i]) + "x",
                        m.type == TimelineMarker::Type::Loop,
                        m.loopRepeats == repeatChoices[i]);
    }
    repMenu.addItem(200, "Custom...", m.type == TimelineMarker::Type::Loop);
    menu.addSubMenu("Loop Repeats", repMenu, m.type == TimelineMarker::Type::Loop);

    menu.addSeparator();
    menu.addItem(2, "Delete");

    juce::Component::SafePointer<TimeLine> safe(this);
    menu.showMenuAsync(juce::PopupMenu::Options(),
        [safe, markerIndex](int result) {
            if (safe == nullptr || result == 0) return;
            if (markerIndex < 0 || markerIndex >= (int) safe->markers.size()) return;
            auto& mk = safe->markers[(size_t) markerIndex];
            switch (result)
            {
                case 1: safe->renameMarker(markerIndex); return;
                case 2: safe->removeMarker(markerIndex); return;
                case 10: mk.type = TimelineMarker::Type::Normal; break;
                case 11: mk.type = TimelineMarker::Type::Loop;
                         if (mk.loopRepeats < 1) mk.loopRepeats = 1;
                         break;
                case 200: safe->setLoopRepeats(markerIndex); return;
                default:
                    if (result >= 100 && result < 200) {
                        mk.loopRepeats = result - 100;
                        mk.type        = TimelineMarker::Type::Loop;
                    }
                    break;
            }
            safe->repaint();
            safe->notifyChanged();
        });
}

void TimeLine::renameMarker(int index)
{
    if (index < 0 || index >= (int) markers.size()) return;

    auto* aw = new juce::AlertWindow("Marker Name",
                                      "Enter a new name for this marker:",
                                      juce::AlertWindow::NoIcon);
    aw->setLookAndFeel(&getLookAndFeel());
    aw->addTextEditor("name", markers[(size_t) index].name);
    aw->addButton("OK",     1, juce::KeyPress(juce::KeyPress::returnKey));
    aw->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    juce::Component::SafePointer<TimeLine> safe(this);
    aw->enterModalState(true,
        juce::ModalCallbackFunction::create([safe, index, aw](int result) {
            if (safe != nullptr && result == 1
                && index >= 0 && index < (int) safe->markers.size())
            {
                safe->markers[(size_t) index].name = aw->getTextEditorContents("name");
                safe->repaint();
                safe->notifyChanged();
            }
        }),
        true /* deleteWhenDismissed */);
}

void TimeLine::setLoopRepeats(int index)
{
    if (index < 0 || index >= (int) markers.size()) return;

    auto* aw = new juce::AlertWindow("Loop Repeats",
                                      "How many times should the section from the "
                                      "previous marker be repeated?",
                                      juce::AlertWindow::NoIcon);
    aw->setLookAndFeel(&getLookAndFeel());
    aw->addTextEditor("n", juce::String(markers[(size_t) index].loopRepeats));
    aw->addButton("OK",     1, juce::KeyPress(juce::KeyPress::returnKey));
    aw->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    juce::Component::SafePointer<TimeLine> safe(this);
    aw->enterModalState(true,
        juce::ModalCallbackFunction::create([safe, index, aw](int result) {
            if (safe != nullptr && result == 1
                && index >= 0 && index < (int) safe->markers.size())
            {
                const int n = juce::jmax(1, aw->getTextEditorContents("n").getIntValue());
                auto& mk = safe->markers[(size_t) index];
                mk.loopRepeats = n;
                mk.type        = TimelineMarker::Type::Loop;
                safe->repaint();
                safe->notifyChanged();
            }
        }),
        true /* deleteWhenDismissed */);
}
