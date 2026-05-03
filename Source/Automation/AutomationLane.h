#pragma once

#include "AutomationTypes.h"

namespace synthlab { namespace automation {

// Datenmodell EINER Automation Lane.
//
// Eigentum: AutomationModel (per Track). Es gibt fuer einen Parameter genau
// EINE Lane oder keine. Lanes haben eine stabile Uuid, die ueber Save/Load
// erhalten bleibt — sie sind NICHT positions-abhaengig.
//
// Punkt-Sortierung wird beim Einfuegen erzwungen. Lookups arbeiten in O(log n).
class Lane
{
public:
    Lane() = default;
    Lane (juce::Uuid id_, juce::String parameterId_)
        : id (id_), parameterId (std::move (parameterId_)) {}

    const juce::Uuid&    getId()          const noexcept { return id; }
    const juce::String&  getParameterId() const noexcept { return parameterId; }

    Mode  getMode()           const noexcept { return mode; }
    void  setMode (Mode m)          noexcept { mode = m; }

    bool  isSampleAccurate()  const noexcept { return sampleAccurate; }
    void  setSampleAccurate (bool b) noexcept { sampleAccurate = b; }

    const std::vector<Point>& getPoints() const noexcept { return points; }
    bool                      isEmpty()   const noexcept { return points.empty(); }
    int                       size()      const noexcept { return (int) points.size(); }

    // Fuegt einen Punkt ein. Wenn bei sampleTime bereits einer existiert,
    // wird er ueberschrieben. Reihenfolge bleibt korrekt.
    void addOrReplacePoint (juce::int64 sampleTime, float value,
                            CurveType curve = CurveType::Linear)
    {
        Point p { sampleTime, juce::jlimit (0.0f, 1.0f, value), curve };
        auto it = std::lower_bound (points.begin(), points.end(), sampleTime,
                                    [] (const Point& a, juce::int64 t)
                                    { return a.sampleTime < t; });
        if (it != points.end() && it->sampleTime == sampleTime)
            *it = p;
        else
            points.insert (it, p);
    }

    void removePointAt (juce::int64 sampleTime) noexcept
    {
        auto it = std::lower_bound (points.begin(), points.end(), sampleTime,
                                    [] (const Point& a, juce::int64 t)
                                    { return a.sampleTime < t; });
        if (it != points.end() && it->sampleTime == sampleTime)
            points.erase (it);
    }

    void clear() noexcept { points.clear(); }

    // Linear-interpolierter Wert an `sampleTime`. Bei leeren Lanes gibt
    // `defaultValue` zurueck. Bei Step-Curve haelt der vorherige Wert bis
    // zum naechsten Punkt; Hold ist ein Synonym dafuer.
    float valueAt (juce::int64 sampleTime, float defaultValue = 0.0f) const noexcept
    {
        if (points.empty())                       return defaultValue;
        if (sampleTime <= points.front().sampleTime) return points.front().value;
        if (sampleTime >= points.back().sampleTime)  return points.back().value;

        auto upper = std::upper_bound (points.begin(), points.end(), sampleTime,
                                       [] (juce::int64 t, const Point& a)
                                       { return t < a.sampleTime; });
        if (upper == points.begin()) return upper->value;
        auto lower = upper - 1;

        if (lower->curve == CurveType::Step || lower->curve == CurveType::Hold)
            return lower->value;

        const auto s0 = lower->sampleTime, s1 = upper->sampleTime;
        if (s1 == s0) return lower->value;
        const float v0 = lower->value, v1 = upper->value;
        const float t  = (float) (sampleTime - s0) / (float) (s1 - s0);
        return v0 + (v1 - v0) * t;
    }

    juce::ValueTree toValueTree() const
    {
        juce::ValueTree v ("Lane");
        v.setProperty ("id",             id.toDashedString(),    nullptr);
        v.setProperty ("paramId",        parameterId,            nullptr);
        v.setProperty ("mode",           juce::String (toString (mode)),    nullptr);
        v.setProperty ("sampleAccurate", sampleAccurate,         nullptr);

        juce::ValueTree pts ("Points");
        for (const auto& p : points)
        {
            juce::ValueTree e ("P");
            e.setProperty ("t", (juce::int64) p.sampleTime,             nullptr);
            e.setProperty ("v", (double)       p.value,                  nullptr);
            e.setProperty ("c", juce::String (toString (p.curve)),       nullptr);
            pts.addChild (e, -1, nullptr);
        }
        v.addChild (pts, -1, nullptr);
        return v;
    }

    static Lane fromValueTree (const juce::ValueTree& v)
    {
        Lane l;
        if (! v.hasType ("Lane")) return l;
        l.id            = juce::Uuid (v.getProperty ("id").toString());
        l.parameterId   = v.getProperty ("paramId").toString();
        l.mode          = modeFromString (v.getProperty ("mode").toString());
        l.sampleAccurate = (bool) v.getProperty ("sampleAccurate", false);

        auto pts = v.getChildWithName ("Points");
        l.points.reserve ((size_t) pts.getNumChildren());
        for (int i = 0; i < pts.getNumChildren(); ++i)
        {
            auto e = pts.getChild (i);
            Point p;
            p.sampleTime = (juce::int64) e.getProperty ("t", (juce::int64) 0);
            p.value      = (float)       (double) e.getProperty ("v", 0.0);
            p.curve      = curveFromString (e.getProperty ("c").toString());
            l.points.push_back (p);
        }
        std::sort (l.points.begin(), l.points.end(),
                   [] (const Point& a, const Point& b)
                   { return a.sampleTime < b.sampleTime; });
        return l;
    }

private:
    juce::Uuid    id;
    juce::String  parameterId;
    Mode          mode             = Mode::Read;
    bool          sampleAccurate   = false;
    std::vector<Point> points;
};

}}
