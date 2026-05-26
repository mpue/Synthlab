//============================================================================
//  DockTypes.h
//  Part of the header-only Docking Windows framework (JUCE based).
//
//  Common types, identifiers and theming used by every other docking header.
//============================================================================
#pragma once

#include <JuceHeader.h>

namespace docking
{

// Forward declarations so other headers can reference these types via this file.
class DockablePanel;
class DockNode;
class DockSplitNode;
class DockTabNode;
class DockManager;
class FloatingDockWindow;
class DockOverlay;

//----------------------------------------------------------------------------
//  Enums
//----------------------------------------------------------------------------

/** Side of a drop target.  Center means "tabify into the target tab group". */
enum class DockSide
{
    None    = 0,
    Center  = 1,
    Left    = 2,
    Right   = 3,
    Top     = 4,
    Bottom  = 5
};

/** Orientation of a split container.
    Horizontal => children placed left-to-right (vertical splitter lines).
    Vertical   => children placed top-to-bottom (horizontal splitter lines). */
enum class SplitOrientation
{
    Horizontal,
    Vertical
};

/** Whether a drop target refers to the outer edge of the main dock area
    or to a specific inner tab node. */
enum class DockTargetScope
{
    None,
    Outer,
    Inner
};

//----------------------------------------------------------------------------
//  ValueTree identifiers used for state persistence
//----------------------------------------------------------------------------
namespace ids
{
    inline const juce::Identifier Layout       { "Layout"       };
    inline const juce::Identifier Root         { "Root"         };
    inline const juce::Identifier Split        { "Split"        };
    inline const juce::Identifier Tabs         { "Tabs"         };
    inline const juce::Identifier Orientation  { "orientation"  };
    inline const juce::Identifier Sizes        { "sizes"        };
    inline const juce::Identifier PanelId      { "panelId"      };
    inline const juce::Identifier ActiveIndex  { "activeIndex"  };
    inline const juce::Identifier Floating     { "Floating"     };
    inline const juce::Identifier Window       { "Window"       };
    inline const juce::Identifier X            { "x"            };
    inline const juce::Identifier Y            { "y"            };
    inline const juce::Identifier W            { "w"            };
    inline const juce::Identifier H            { "h"            };
    inline const juce::Identifier Panel        { "Panel"        };
}

//----------------------------------------------------------------------------
//  Theme
//----------------------------------------------------------------------------

/** Simple, mutable colour/metric bundle.  The manager owns one and every
    component asks for it via DockablePanel::getTheme().  Change values at any
    time and call DockManager::repaintAll() to refresh. */
struct DockTheme
{
    juce::Colour background        { 0xff2b2b2b };
    juce::Colour headerInactive    { 0xff3c3f41 };
    juce::Colour headerActive      { 0xff214283 };
    juce::Colour headerText        { 0xfff0f0f0 };
    juce::Colour tabInactive       { 0xff3c3f41 };
    juce::Colour tabActive         { 0xff214283 };
    juce::Colour tabHover          { 0xff4b6eaf };
    juce::Colour tabText           { 0xffe0e0e0 };
    juce::Colour border            { 0xff1e1e1e };
    juce::Colour splitter          { 0xff1e1e1e };
    juce::Colour splitterHover     { 0xff3a78c8 };
    juce::Colour overlayHint       { juce::Colour::fromRGBA (60, 130, 220,  90) };
    juce::Colour overlayHintBorder { juce::Colour::fromRGBA (60, 130, 220, 255) };
    juce::Colour guideFill         { 0xfff4f4f4 };
    juce::Colour guideBorder       { 0xff202020 };
    juce::Colour guideHover        { 0xff60a0ff };

    int headerHeight = 22;
    int tabHeight    = 22;
    int splitterSize = 4;
    int guideSize    = 34;   // size of an individual guide button
    int guideGap     = 4;    // gap between the five inner guide buttons
};

//----------------------------------------------------------------------------
//  Small helpers
//----------------------------------------------------------------------------

/** Pixel-snapped rounded rectangle for crisp overlay drawing. */
inline void fillSnappedRect (juce::Graphics& g, juce::Rectangle<int> r)
{
    g.fillRect (r);
}

/** Convert string <-> SplitOrientation for ValueTree serialisation. */
inline const char* toString (SplitOrientation o) noexcept
{
    return o == SplitOrientation::Horizontal ? "horizontal" : "vertical";
}

inline SplitOrientation orientationFromString (const juce::String& s) noexcept
{
    return s == "vertical" ? SplitOrientation::Vertical
                           : SplitOrientation::Horizontal;
}

} // namespace docking
