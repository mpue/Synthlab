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

        juce::Path path;
        bool first = true;
        for (const auto& pt : points)
        {
            const float x = (float) sampleToPixel(pt.sample);
            const float y = valueToY(pt.value);
            if (first) { path.startNewSubPath(x, y); first = false; }
            else        path.lineTo(x, y);
        }

        // Sattgrüne, deutlich sichtbare Linie über den Regions.
        g.setColour(juce::Colour(0xff3ed16a));
        g.strokePath(path, juce::PathStrokeType(3.0f,
            juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Quadratische Handles an jedem Stützwert — kontrastreich gefüllt
        // mit dünnem Rahmen, damit sie auch über hellen Wellenformen lesbar
        // bleiben.
        const float handle = 7.0f;
        for (const auto& pt : points)
        {
            const float x = (float) sampleToPixel(pt.sample);
            const float y = valueToY(pt.value);
            const juce::Rectangle<float> r (x - handle * 0.5f,
                                            y - handle * 0.5f,
                                            handle, handle);
            g.setColour(juce::Colour(0xff3ed16a));
            g.fillRect(r);
            g.setColour(juce::Colours::black);
            g.drawRect(r, 1.0f);
        }
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (!overlayActive) return;

        if (e.mods.isRightButtonDown())
        {
            // Rechtsklick löscht den nächstgelegenen Punkt im Pixel-Radius.
            const long sample = pixelToSample(e.x);
            removeNearest(sample, 6);
            repaint();
            return;
        }

        addOrReplacePoint(pixelToSample(e.x), yToValue(e.y));
        repaint();
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (!overlayActive) return;
        if (e.mods.isRightButtonDown())
        {
            const long sample = pixelToSample(e.x);
            removeNearest(sample, 6);
            repaint();
            return;
        }
        addOrReplacePoint(pixelToSample(e.x), yToValue(e.y));
        repaint();
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
            points.erase(best);
    }

    Track* track = nullptr;
    double sampleRate = 44100.0;
    bool overlayActive = false;
    int  paramIndex = -1;
    float zoom = 40.0f;
    std::vector<Point> points;

    static constexpr int headerHeight = 16;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomationLane)
};
