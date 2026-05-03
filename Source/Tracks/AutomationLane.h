/*
  ==============================================================================

    AutomationLane.h
    Per-Track-Overlay zum Zeichnen einer Parameter-Automation. Liegt als Child
    über den Regions des Tracks und wird vom TrackControl-Toggle ein-/ausge-
    blendet. Der Anwender wählt im TrackControl per ComboBox einen automati-
    sierbaren Plugin-Parameter; mit gedrückter Maustaste lässt sich dann ein
    Verlauf in den Track zeichnen. Während der Wiedergabe ruft Track::setSample-
    Position() applyAt(samplePosition) auf, was den Wert linear interpoliert
    und über setValueNotifyingHost() ans Plugin schickt.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "../Project/Project.h"
#include <set>

class Track;

class AutomationLane : public juce::Component
{
public:
    struct Point {
        long  sample = 0;   // Sample-Offset im Track-Timeline
        float value  = 0.f; // normalisiert 0..1
    };

    AutomationLane(Track* owner, double sampleRate)
        : track(owner), sampleRate(sampleRate)
    {
        setOpaque(false);
        setInterceptsMouseClicks(false, false); // erst bei aktivem Overlay an
        setWantsKeyboardFocus(true);
    }

    ~AutomationLane() override = default;

    void setOverlayActive(bool on)
    {
        overlayActive = on;
        // Wenn aktiv, fängt das Overlay alle Klicks ab — Regions sollen nur in
        // ausgeschaltetem Zustand selektierbar bleiben. setAlwaysOnTop sorgt
        // dafür, dass spätere addRegion()-Aufrufe (die toFront auf die neue
        // Region machen) die Lane nicht unter die Regions schieben.
        setInterceptsMouseClicks(on, false);
        setAlwaysOnTop(on);
        if (on)
            toFront(false);
        repaint();
    }

    bool isOverlayActive() const { return overlayActive; }

    // Setzt den Parameter-Index relativ zur Plugin-Parameterliste. -1 = keiner.
    void setParameterIndex(int idx) { paramIndex = idx; repaint(); }
    int  getParameterIndex() const  { return paramIndex; }

    void setZoom(float z) { zoom = z; repaint(); }
    float getZoom() const { return zoom; }

    void clearPoints() { points.clear(); repaint(); }

    const std::vector<Point>& getPoints() const { return points; }

    // Während der Wiedergabe vom Track aufgerufen. Liest den interpolierten
    // Wert an `sample` und ruft setValueNotifyingHost() am Ziel-Parameter.
    void applyAt(long sample);

    juce::AudioProcessorParameter* getTargetParameter() const;

    void paint(juce::Graphics& g) override
    {
        if (!overlayActive) return;

        auto bounds = getLocalBounds().toFloat();

        // Halbtransparenter Schleier, damit klar ist: jetzt im Automation-Modus.
        g.setColour(juce::Colour(0x33000000));
        g.fillRect(bounds);

        // Header-Text mit dem aktuellen Parameter-Namen.
        g.setColour(juce::Colours::white.withAlpha(0.8f));
        g.setFont(juce::Font(11.0f, juce::Font::bold));
        juce::String label = "Automation";
        if (auto* p = getTargetParameter())
            label += " : " + p->getName(40);
        g.drawText(label, 4, 2, getWidth() - 8, 14, juce::Justification::topLeft, true);

        // Verlauf zeichnen.
        if (points.size() == 0)
        {
            g.setColour(juce::Colours::white.withAlpha(0.35f));
            g.drawText("Mit der Maus zeichnen…", getLocalBounds(),
                       juce::Justification::centred, false);
            return;
        }

        // Visibility culling: berechne den Sample-Bereich, der sichtbar ist.
        // Das haelt das Path-Drawing auch bei tausenden Punkten flott
        // (>= 60 fps target, siehe Phase-2-Akzeptanzkriterium).
        const auto clip = g.getClipBounds();
        const long visStart = pixelToSample(juce::jmax(0, clip.getX() - 4));
        const long visEnd   = pixelToSample(clip.getRight() + 4);

        // ersten Punkt links der Sicht und ersten rechts der Sicht behalten,
        // damit die Linie an den Kanten nicht abrupt endet.
        auto firstVis = std::lower_bound(points.begin(), points.end(), visStart,
            [](const Point& p, long t){ return p.sample < t; });
        if (firstVis != points.begin()) --firstVis;
        auto lastVis = std::upper_bound(points.begin(), points.end(), visEnd,
            [](long t, const Point& p){ return t < p.sample; });
        if (lastVis != points.end()) ++lastVis;

        juce::Path path;
        bool first = true;
        for (auto it = firstVis; it != lastVis; ++it)
        {
            const float x = (float) sampleToPixel(it->sample);
            const float y = valueToY(it->value);
            if (first) { path.startNewSubPath(x, y); first = false; }
            else        path.lineTo(x, y);
        }

        g.setColour(juce::Colour(0xff3ed16a));
        g.strokePath(path, juce::PathStrokeType(3.0f,
            juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Handles nur fuer sichtbare Punkte. Selektierte gelb gefuellt.
        const float handle = 7.0f;
        for (auto it = firstVis; it != lastVis; ++it)
        {
            const int idx = (int)(it - points.begin());
            const float x = (float) sampleToPixel(it->sample);
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

        // Optionales Auswahl-Rechteck (Drag-Box) waehrend der Selektion.
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

        // Rechtsklick: naechstgelegenen Punkt loeschen.
        if (e.mods.isRightButtonDown())
        {
            removeNearest(pixelToSample(e.x), 6);
            repaint();
            return;
        }

        // Treffer auf existierenden Punkt -> Selektion + Drag vorbereiten.
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
            captureDragOriginsForSelected();
            repaint();
            return;
        }

        // Klick mit Modifier auf leeren Bereich: Box-Auswahl beginnen.
        if (e.mods.isShiftDown() || e.mods.isAltDown())
        {
            selectionBoxActive = true;
            selectionBoxStart  = { e.x, e.y };
            selectionBoxEnd    = { e.x, e.y };
            dragMode           = DragMode::selectionBox;
            repaint();
            return;
        }

        // Sonst: neuen Punkt setzen und sofort selektieren, bereit zum Drag.
        selectedIndices.clear();
        const long s = applySnap(pixelToSample(e.x));
        addOrReplacePoint(s, yToValue(e.y));
        const int newIdx = indexAtSample(s);
        if (newIdx >= 0) selectedIndices.insert(newIdx);
        dragMode    = DragMode::movePoints;
        dragStartX  = e.x;
        dragStartY  = e.y;
        captureDragOriginsForSelected();
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

        if (dragMode == DragMode::movePoints && !selectedIndices.empty())
        {
            const int dxPix = e.x - dragStartX;
            const int dyPix = e.y - dragStartY;

            // Punkte verschieben. Sample-Reihenfolge wird durch
            // rebuildAfterMove() korrigiert (kann Indizes umsortieren).
            for (auto& kv : dragOrigins)
            {
                const int origIdx = kv.first;
                if (origIdx < 0 || origIdx >= (int) points.size()) continue;
                const long origSample = kv.second.first;
                const float origValue = kv.second.second;
                const int origPx      = sampleToPixel(origSample);
                const float origPy    = valueToY(origValue);
                const long newSample  = applySnap(pixelToSample(origPx + dxPix));
                const float newValue  = yToValue((int) origPy + dyPix);
                points[(size_t) origIdx].sample = juce::jmax(0L, newSample);
                points[(size_t) origIdx].value  = juce::jlimit(0.0f, 1.0f, newValue);
            }
            rebuildAfterMove();
            repaint();
        }
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        if (dragMode == DragMode::selectionBox) selectionBoxActive = false;
        dragMode = DragMode::none;
        dragOrigins.clear();
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
            for (int i = 0; i < (int) points.size(); ++i) selectedIndices.insert(i);
            repaint();
            return true;
        }
        return false;
    }

    // ValueTree-Persistenz (Save/Load durch das Projekt).
    juce::ValueTree toValueTree() const
    {
        juce::ValueTree v("Automation");
        v.setProperty("active",       overlayActive, nullptr);
        v.setProperty("paramIndex",   paramIndex,    nullptr);
        juce::ValueTree pts("Points");
        for (const auto& p : points)
        {
            juce::ValueTree e("P");
            e.setProperty("s", (int) p.sample, nullptr);
            e.setProperty("v", p.value,        nullptr);
            pts.addChild(e, -1, nullptr);
        }
        v.addChild(pts, -1, nullptr);
        return v;
    }

    void fromValueTree(const juce::ValueTree& v)
    {
        if (!v.hasType("Automation")) return;
        overlayActive = (bool) v.getProperty("active", false);
        paramIndex    = (int)  v.getProperty("paramIndex", -1);
        points.clear();
        auto pts = v.getChildWithName("Points");
        for (int i = 0; i < pts.getNumChildren(); ++i)
        {
            auto e = pts.getChild(i);
            Point p;
            p.sample = (long)  (int)   e.getProperty("s", 0);
            p.value  = (float) (double)e.getProperty("v", 0.0);
            points.push_back(p);
        }
        std::sort(points.begin(), points.end(),
                  [](const Point& a, const Point& b){ return a.sample < b.sample; });
        setInterceptsMouseClicks(overlayActive, false);
        repaint();
    }

private:
    long pixelToSample(int x) const
    {
        if (zoom <= 0.0f) return 0;
        long s = (long) (((double) x / (double) zoom) * sampleRate);
        if (s < 0) s = 0;
        return s;
    }

    int sampleToPixel(long sample) const
    {
        if (sampleRate <= 0.0) return 0;
        return (int) (((double) sample / sampleRate) * (double) zoom);
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

    void addOrReplacePoint(long sample, float value)
    {
        // Punkte werden in Sample-Buckets gespeichert, deren Breite einem
        // Pixel-Quantum entspricht — sonst würde ein einzelner Drag hunderte
        // Punkte pro Pixel ablegen und die Wiedergabe verteuern.
        long bucket = (long)((double) sampleRate / juce::jmax(1.0f, zoom));
        if (bucket < 1) bucket = 1;
        const long key = (sample / bucket) * bucket;

        // Existierenden Bucket aktualisieren oder einsortieren.
        auto it = std::lower_bound(points.begin(), points.end(), key,
            [](const Point& p, long k){ return p.sample < k; });

        if (it != points.end() && it->sample == key)
        {
            it->value = value;
        }
        else
        {
            Point p; p.sample = key; p.value = value;
            points.insert(it, p);
        }
    }

    void removeNearest(long sample, int pixelRadius)
    {
        if (points.empty()) return;
        const int xRef = sampleToPixel(sample);
        auto best = points.end();
        int bestDist = pixelRadius;
        for (auto it = points.begin(); it != points.end(); ++it)
        {
            const int dx = std::abs(sampleToPixel(it->sample) - xRef);
            if (dx <= bestDist) { bestDist = dx; best = it; }
        }
        if (best != points.end())
        {
            const int idx = (int)(best - points.begin());
            points.erase(best);
            removeIndexFromSelection(idx);
        }
    }

    int hitTestPoint(int x, int y, int pixelRadius) const
    {
        int best = -1;
        int bestDist = pixelRadius * pixelRadius + 1;
        for (size_t i = 0; i < points.size(); ++i)
        {
            const int px = sampleToPixel(points[i].sample);
            if (std::abs(px - x) > pixelRadius) continue;
            const int py = (int) valueToY(points[i].value);
            const int dx = px - x, dy = py - y;
            const int d  = dx * dx + dy * dy;
            if (d < bestDist) { bestDist = d; best = (int) i; }
        }
        return best;
    }

    int indexAtSample(long sample) const
    {
        for (size_t i = 0; i < points.size(); ++i)
            if (points[i].sample == sample) return (int) i;
        return -1;
    }

    long applySnap(long sample) const
    {
        auto* pr = Project::getInstance();
        if (pr == nullptr || !pr->isSnapToGridEnabled()) return sample;
        return pr->snapSampleToGrid(sample, zoom, false);
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
        const auto box = currentSelectionBox();
        for (size_t i = 0; i < points.size(); ++i)
        {
            const int px = sampleToPixel(points[i].sample);
            const int py = (int) valueToY(points[i].value);
            if (box.contains(px, py)) selectedIndices.insert((int) i);
        }
    }

    void captureDragOriginsForSelected()
    {
        dragOrigins.clear();
        for (int i : selectedIndices)
            dragOrigins[i] = { points[(size_t) i].sample, points[(size_t) i].value };
    }

    // Nach einem Drag koennen sich die Sample-Werte so verschoben haben, dass
    // die globale Reihenfolge nicht mehr stimmt. Hier neu sortieren und die
    // Index-basierte Selektion sauber neu zusammensetzen.
    void rebuildAfterMove()
    {
        // Snapshot aktueller Selektion mit Identitaet ueber sample/value.
        std::vector<std::pair<long,float>> selSnapshot;
        for (int i : selectedIndices)
            if (i >= 0 && i < (int) points.size())
                selSnapshot.emplace_back(points[(size_t) i].sample,
                                         points[(size_t) i].value);

        std::sort(points.begin(), points.end(),
                  [](const Point& a, const Point& b){ return a.sample < b.sample; });

        selectedIndices.clear();
        for (const auto& key : selSnapshot)
            for (size_t i = 0; i < points.size(); ++i)
                if (points[i].sample == key.first && points[i].value == key.second)
                    { selectedIndices.insert((int) i); break; }
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
        // In absteigender Reihenfolge loeschen, damit Indizes gueltig bleiben.
        std::vector<int> toDel(selectedIndices.begin(), selectedIndices.end());
        std::sort(toDel.begin(), toDel.end(), std::greater<int>());
        for (int i : toDel)
            if (i >= 0 && i < (int) points.size())
                points.erase(points.begin() + i);
        selectedIndices.clear();
        repaint();
    }

    enum class DragMode { none, movePoints, selectionBox };

    Track* track = nullptr;
    double sampleRate = 44100.0;
    bool overlayActive = false;
    int  paramIndex = -1;
    float zoom = 40.0f;
    std::vector<Point> points;

    std::set<int> selectedIndices;
    std::map<int, std::pair<long, float>> dragOrigins;
    DragMode dragMode = DragMode::none;
    int      dragStartX = 0, dragStartY = 0;
    bool     selectionBoxActive = false;
    juce::Point<int> selectionBoxStart, selectionBoxEnd;

    static constexpr int headerHeight = 16;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomationLane)
};
