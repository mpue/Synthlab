/*
  ==============================================================================

    AutomationLane.h
    Per-Track-Overlay zum Zeichnen einer Parameter-Automation. Liegt als Child
    ueber den Regions des Tracks und wird vom TrackControl-Toggle ein-/aus-
    geblendet. Der Anwender waehlt im TrackControl per ComboBox einen automati-
    sierbaren Plugin-Parameter; mit gedrueckter Maustaste laesst sich dann ein
    Verlauf in den Track zeichnen.

    Datenhaltung: das Overlay haelt KEINE eigenen Punkte mehr — es ist an
    eine `synthlab::automation::Lane` aus dem Track-Modell gebunden
    (`Track::getAutomationModel()`). Track::setAutomationParameterIndex(idx)
    waehlt die zu zeigende Lane, falls noch nicht vorhanden wird sie idempotent
    angelegt. Wenn keine Lane gebunden ist, wird ein interner Scratch-Lane
    benutzt, damit Mausinteraktionen vor der Parameter-Auswahl nichts ins Leere
    schreiben.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "../Project/Project.h"
#include "../Automation/AutomationLane.h"
#include <set>

class Track;

class AutomationLane : public juce::Component
{
public:
    using Point = synthlab::automation::Point;

    AutomationLane(Track* owner, double sampleRate)
        : track(owner), sampleRate(sampleRate)
    {
        setOpaque(false);
        setInterceptsMouseClicks(false, false);
        setWantsKeyboardFocus(true);
    }

    ~AutomationLane() override = default;

    void setOverlayActive(bool on)
    {
        overlayActive = on;
        setInterceptsMouseClicks(on, false);
        setAlwaysOnTop(on);
        if (on) toFront(false);
        repaint();
    }

    bool isOverlayActive() const { return overlayActive; }

    void setParameterIndex(int idx) { paramIndex = idx; repaint(); }
    int  getParameterIndex() const  { return paramIndex; }

    void setZoom(float z) { zoom = z; repaint(); }
    float getZoom() const { return zoom; }

    // ----- Lane-Binding -------------------------------------------------------
    // boundLane darf nullptr sein — dann wird auf scratchLane geschrieben,
    // damit Mausinteraktionen vor der Parameter-Auswahl nichts ins Leere
    // schreiben (sie verschwinden dann eben beim Parameter-Wechsel).
    void setBoundLane(synthlab::automation::Lane* lane)
    {
        boundLane = lane;
        selectedIndices.clear();
        dragSlots.clear();
        repaint();
    }

    synthlab::automation::Lane* getBoundLane() const noexcept { return boundLane; }

    void clearPoints()
    {
        if (auto* l = activeLane()) l->clear();
        selectedIndices.clear();
        repaint();
    }

    const std::vector<Point>& getPoints() const
    {
        return activeLane() != nullptr ? activeLane()->getPoints()
                                       : scratchLane.getPoints();
    }

    // Waehrend der Wiedergabe vom Track aufgerufen. Liest den interpolierten
    // Wert an `sample` und ruft setValueNotifyingHost() am Ziel-Parameter.
    void applyAt(long sample);

    juce::AudioProcessorParameter* getTargetParameter() const;

    void paint(juce::Graphics& g) override
    {
        if (!overlayActive) return;

        auto bounds = getLocalBounds().toFloat();

        g.setColour(juce::Colour(0x33000000));
        g.fillRect(bounds);

        g.setColour(juce::Colours::white.withAlpha(0.8f));
        g.setFont(juce::Font(11.0f, juce::Font::bold));
        juce::String label = "Automation";
        if (auto* p = getTargetParameter())
            label += " : " + p->getName(40);
        g.drawText(label, 4, 2, getWidth() - 8, 14, juce::Justification::topLeft, true);

        const auto& points = getPoints();
        if (points.empty())
        {
            g.setColour(juce::Colours::white.withAlpha(0.35f));
            g.drawText("Mit der Maus zeichnen…", getLocalBounds(),
                       juce::Justification::centred, false);
            return;
        }

        // Visibility culling: nur Punkte im sichtbaren Sample-Bereich zeichnen
        // — haelt die Lane fluessig auch bei tausenden Punkten (Phase 2).
        const auto clip = g.getClipBounds();
        const juce::int64 visStart = pixelToSample(juce::jmax(0, clip.getX() - 4));
        const juce::int64 visEnd   = pixelToSample(clip.getRight() + 4);

        auto firstVis = std::lower_bound(points.begin(), points.end(), visStart,
            [](const Point& p, juce::int64 t){ return p.sampleTime < t; });
        if (firstVis != points.begin()) --firstVis;
        auto lastVis = std::upper_bound(points.begin(), points.end(), visEnd,
            [](juce::int64 t, const Point& p){ return t < p.sampleTime; });
        if (lastVis != points.end()) ++lastVis;

        juce::Path path;
        bool first = true;
        for (auto it = firstVis; it != lastVis; ++it)
        {
            const float x = (float) sampleToPixel(it->sampleTime);
            const float y = valueToY(it->value);
            if (first) { path.startNewSubPath(x, y); first = false; }
            else        path.lineTo(x, y);
        }

        g.setColour(juce::Colour(0xff3ed16a));
        g.strokePath(path, juce::PathStrokeType(3.0f,
            juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        const float handle = 7.0f;
        for (auto it = firstVis; it != lastVis; ++it)
        {
            const int idx = (int)(it - points.begin());
            const float x = (float) sampleToPixel(it->sampleTime);
            const float y = valueToY(it->value);
            const juce::Rectangle<float> r (x - handle * 0.5f,
                                            y - handle * 0.5f,
                                            handle, handle);
            const bool sel = selectedIndices.count(idx) > 0;
            g.setColour(sel ? juce::Colours::yellow : juce::Colour(0xff3ed16a));
            g.fillRect(r);
            g.setColour(juce::Colours::black);
            g.drawRect(r, 1.0f);
        }

        if (selectionBoxActive)
        {
            const auto box = currentSelectionBox().toFloat();
            g.setColour(juce::Colours::yellow.withAlpha(0.15f));
            g.fillRect(box);
            g.setColour(juce::Colours::yellow.withAlpha(0.7f));
            g.drawRect(box, 1.0f);
        }
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (!overlayActive) return;
        grabKeyboardFocus();

        if (e.mods.isRightButtonDown())
        {
            removeNearest(pixelToSample(e.x), 6);
            repaint();
            return;
        }

        const int hit = hitTestPoint(e.x, e.y, 6);
        if (hit >= 0)
        {
            if (e.mods.isShiftDown() || e.mods.isCommandDown())
            {
                if (selectedIndices.count(hit) > 0) selectedIndices.erase(hit);
                else                                selectedIndices.insert(hit);
            }
            else if (selectedIndices.count(hit) == 0)
            {
                selectedIndices.clear();
                selectedIndices.insert(hit);
            }
            dragMode      = DragMode::movePoints;
            dragStartX    = e.x;
            dragStartY    = e.y;
            captureDragSlotsForSelected();
            repaint();
            return;
        }

        if (e.mods.isShiftDown() || e.mods.isAltDown())
        {
            selectionBoxActive = true;
            selectionBoxStart  = { e.x, e.y };
            selectionBoxEnd    = { e.x, e.y };
            dragMode           = DragMode::selectionBox;
            repaint();
            return;
        }

        selectedIndices.clear();
        const juce::int64 s = applySnap(pixelToSample(e.x));
        addOrReplacePoint(s, yToValue(e.y));
        const int newIdx = indexAtSample(s);
        if (newIdx >= 0) selectedIndices.insert(newIdx);
        dragMode    = DragMode::movePoints;
        dragStartX  = e.x;
        dragStartY  = e.y;
        captureDragSlotsForSelected();
        repaint();
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (!overlayActive) return;

        if (e.mods.isRightButtonDown())
        {
            removeNearest(pixelToSample(e.x), 6);
            repaint();
            return;
        }

        if (dragMode == DragMode::selectionBox)
        {
            selectionBoxEnd = { e.x, e.y };
            applySelectionBox(e.mods.isCommandDown() || e.mods.isShiftDown());
            repaint();
            return;
        }

        if (dragMode == DragMode::movePoints && !dragSlots.empty())
        {
            auto* lane = activeLane();
            if (lane == nullptr) return;

            // Delta IMMER relativ zur Original-Mausposition. dragStartX/Y und
            // die Origin-Positionen in dragSlots[] werden waehrend des Drags
            // NICHT veraendert — sonst akkumuliert sich die Bewegung.
            const int dxPix = e.x - dragStartX;
            const int dyPix = e.y - dragStartY;

            // Erst die Punkte loeschen, die wir im LETZTEN Frame platziert
            // haben (currentSample). Beim allerersten Frame sind currentSample
            // == origSample, also werden die Originale weggeraeumt.
            for (const auto& s : dragSlots)
                lane->removePointAt(s.currentSample);

            // Ziel-Position aus der ORIGINAL-Position + delta-from-start
            // berechnen — so bleibt 1px Mausbewegung == 1px Punktbewegung.
            for (auto& s : dragSlots)
            {
                const int origPx = sampleToPixel(s.origSample);
                const float origPy = valueToY(s.origValue);
                const juce::int64 newSample = juce::jmax((juce::int64) 0,
                                                         applySnap(pixelToSample(origPx + dxPix)));
                const float newValue        = juce::jlimit(0.0f, 1.0f,
                                                           yToValue((int) origPy + dyPix));
                lane->addOrReplacePoint(newSample, newValue);
                s.currentSample = newSample;
                s.currentValue  = newValue;
            }

            // Selektions-Indizes auf die neuen Punkt-Positionen mappen.
            selectedIndices.clear();
            const auto& pts = lane->getPoints();
            std::set<juce::int64> targetSamples;
            for (const auto& s : dragSlots) targetSamples.insert(s.currentSample);
            for (size_t i = 0; i < pts.size(); ++i)
                if (targetSamples.count(pts[i].sampleTime) > 0)
                    selectedIndices.insert((int) i);

            repaint();
        }
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        if (dragMode == DragMode::selectionBox) selectionBoxActive = false;
        dragMode = DragMode::none;
        dragSlots.clear();
        repaint();
    }

    bool keyPressed(const juce::KeyPress& key) override
    {
        if (!overlayActive) return false;
        if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
        {
            deleteSelected();
            return true;
        }
        if (key == juce::KeyPress('a', juce::ModifierKeys::commandModifier, 0))
        {
            selectedIndices.clear();
            const auto& pts = getPoints();
            for (int i = 0; i < (int) pts.size(); ++i) selectedIndices.insert(i);
            repaint();
            return true;
        }
        return false;
    }

private:
    synthlab::automation::Lane* activeLane()
    {
        return boundLane != nullptr ? boundLane : &scratchLane;
    }
    const synthlab::automation::Lane* activeLane() const
    {
        return boundLane != nullptr ? boundLane : &scratchLane;
    }

    juce::int64 pixelToSample(int x) const
    {
        if (zoom <= 0.0f) return 0;
        juce::int64 s = (juce::int64)(((double) x / (double) zoom) * sampleRate);
        if (s < 0) s = 0;
        return s;
    }

    int sampleToPixel(juce::int64 sample) const
    {
        if (sampleRate <= 0.0) return 0;
        return (int)(((double) sample / sampleRate) * (double) zoom);
    }

    float yToValue(int y) const
    {
        const int top = headerHeight;
        const int h   = juce::jmax(1, getHeight() - top - 2);
        float v = 1.0f - (float)(y - top) / (float) h;
        return juce::jlimit(0.0f, 1.0f, v);
    }

    float valueToY(float v) const
    {
        const int top = headerHeight;
        const int h   = juce::jmax(1, getHeight() - top - 2);
        return (float) top + (1.0f - juce::jlimit(0.0f, 1.0f, v)) * (float) h;
    }

    void addOrReplacePoint(juce::int64 sample, float value)
    {
        // Pixel-Bucket-Quantisierung — sonst legt ein Drag hunderte Punkte
        // pro Pixel ab.
        juce::int64 bucket = (juce::int64)((double) sampleRate / juce::jmax(1.0f, zoom));
        if (bucket < 1) bucket = 1;
        const juce::int64 key = (sample / bucket) * bucket;
        if (auto* l = activeLane()) l->addOrReplacePoint(key, value);
    }

    void removeNearest(juce::int64 sample, int pixelRadius)
    {
        auto* lane = activeLane();
        if (lane == nullptr || lane->isEmpty()) return;
        const auto& pts = lane->getPoints();
        const int xRef = sampleToPixel(sample);
        int best = -1;
        int bestDist = pixelRadius;
        for (size_t i = 0; i < pts.size(); ++i)
        {
            const int dx = std::abs(sampleToPixel(pts[i].sampleTime) - xRef);
            if (dx <= bestDist) { bestDist = dx; best = (int) i; }
        }
        if (best >= 0)
        {
            const auto t = pts[(size_t) best].sampleTime;
            lane->removePointAt(t);
            removeIndexFromSelection(best);
        }
    }

    int hitTestPoint(int x, int y, int pixelRadius) const
    {
        const auto& points = getPoints();
        int best = -1;
        int bestDist = pixelRadius * pixelRadius + 1;
        for (size_t i = 0; i < points.size(); ++i)
        {
            const int px = sampleToPixel(points[i].sampleTime);
            if (std::abs(px - x) > pixelRadius) continue;
            const int py = (int) valueToY(points[i].value);
            const int dx = px - x, dy = py - y;
            const int d  = dx * dx + dy * dy;
            if (d < bestDist) { bestDist = d; best = (int) i; }
        }
        return best;
    }

    int indexAtSample(juce::int64 sample) const
    {
        const auto& points = getPoints();
        for (size_t i = 0; i < points.size(); ++i)
            if (points[i].sampleTime == sample) return (int) i;
        return -1;
    }

    juce::int64 applySnap(juce::int64 sample) const
    {
        auto* pr = Project::getInstance();
        if (pr == nullptr || !pr->isSnapToGridEnabled()) return sample;
        return (juce::int64) pr->snapSampleToGrid((long) sample, zoom, false);
    }

    juce::Rectangle<int> currentSelectionBox() const
    {
        const int x1 = juce::jmin(selectionBoxStart.x, selectionBoxEnd.x);
        const int y1 = juce::jmin(selectionBoxStart.y, selectionBoxEnd.y);
        const int x2 = juce::jmax(selectionBoxStart.x, selectionBoxEnd.x);
        const int y2 = juce::jmax(selectionBoxStart.y, selectionBoxEnd.y);
        return { x1, y1, x2 - x1, y2 - y1 };
    }

    void applySelectionBox(bool additive)
    {
        if (!additive) selectedIndices.clear();
        const auto& pts = getPoints();
        const auto box = currentSelectionBox();
        for (size_t i = 0; i < pts.size(); ++i)
        {
            const int px = sampleToPixel(pts[i].sampleTime);
            const int py = (int) valueToY(pts[i].value);
            if (box.contains(px, py)) selectedIndices.insert((int) i);
        }
    }

    void captureDragSlotsForSelected()
    {
        dragSlots.clear();
        const auto& pts = getPoints();
        for (int i : selectedIndices)
            if (i >= 0 && i < (int) pts.size())
            {
                DragSlot s;
                s.origSample    = s.currentSample = pts[(size_t) i].sampleTime;
                s.origValue     = s.currentValue  = pts[(size_t) i].value;
                dragSlots.push_back(s);
            }
    }

    void removeIndexFromSelection(int removedIdx)
    {
        std::set<int> next;
        for (int i : selectedIndices)
        {
            if (i == removedIdx) continue;
            next.insert(i > removedIdx ? i - 1 : i);
        }
        selectedIndices.swap(next);
    }

    void deleteSelected()
    {
        if (selectedIndices.empty()) return;
        auto* lane = activeLane();
        if (lane == nullptr) return;
        // Erst die Samplezeiten der zu loeschenden Punkte sammeln, dann jeden
        // einzeln im Lane entfernen — Indizes wuerden sich sonst verschieben.
        const auto& pts = lane->getPoints();
        std::vector<juce::int64> toDel;
        for (int i : selectedIndices)
            if (i >= 0 && i < (int) pts.size())
                toDel.push_back(pts[(size_t) i].sampleTime);
        for (auto t : toDel) lane->removePointAt(t);
        selectedIndices.clear();
        repaint();
    }

    enum class DragMode { none, movePoints, selectionBox };

    Track* track = nullptr;
    double sampleRate = 44100.0;
    bool overlayActive = false;
    int  paramIndex = -1;
    float zoom = 40.0f;

    // Datenhaltung: gebunden an die Lane im Track-Modell, oder Scratch.
    synthlab::automation::Lane* boundLane = nullptr;
    synthlab::automation::Lane  scratchLane;

    std::set<int> selectedIndices;

    // Drag-Tracking pro selektiertem Punkt:
    //   orig*    — Position bei Drag-Start, fest, fuer die Ziel-Berechnung
    //   current* — wo wir den Punkt im LETZTEN Frame platziert haben, fuer
    //              das Loeschen des alten Eintrags vorm Re-Insert
    struct DragSlot {
        juce::int64 origSample    = 0;
        float       origValue     = 0.0f;
        juce::int64 currentSample = 0;
        float       currentValue  = 0.0f;
    };
    std::vector<DragSlot> dragSlots;

    DragMode dragMode = DragMode::none;
    int      dragStartX = 0, dragStartY = 0;
    bool     selectionBoxActive = false;
    juce::Point<int> selectionBoxStart, selectionBoxEnd;

    static constexpr int headerHeight = 16;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomationLane)
};
