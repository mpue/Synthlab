#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

namespace synthlab { namespace automation {

enum class CurveType : int
{
    Linear  = 0,
    Step    = 1,
    Hold    = 2
};

enum class Mode : int
{
    Read  = 0,
    Write = 1,
    Touch = 2,
    Latch = 3
};

inline const char* toString (CurveType c) noexcept
{
    switch (c)
    {
        case CurveType::Linear: return "linear";
        case CurveType::Step:   return "step";
        case CurveType::Hold:   return "hold";
    }
    return "linear";
}

inline CurveType curveFromString (const juce::String& s) noexcept
{
    if (s == "step") return CurveType::Step;
    if (s == "hold") return CurveType::Hold;
    return CurveType::Linear;
}

inline const char* toString (Mode m) noexcept
{
    switch (m)
    {
        case Mode::Read:  return "read";
        case Mode::Write: return "write";
        case Mode::Touch: return "touch";
        case Mode::Latch: return "latch";
    }
    return "read";
}

inline Mode modeFromString (const juce::String& s) noexcept
{
    if (s == "write") return Mode::Write;
    if (s == "touch") return Mode::Touch;
    if (s == "latch") return Mode::Latch;
    return Mode::Read;
}

struct Point
{
    juce::int64 sampleTime = 0;
    float       value      = 0.0f;
    CurveType   curve      = CurveType::Linear;

    bool operator== (const Point& o) const noexcept
    {
        return sampleTime == o.sampleTime
            && value      == o.value
            && curve      == o.curve;
    }
};

}}
