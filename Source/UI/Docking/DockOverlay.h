//============================================================================
//  DockOverlay.h
//  Transparent overlay component drawn on top of the dock area during a
//  drag operation.  Renders:
//      * the diamond of inner guide buttons (centre + 4 sides),
//      * the outer guide buttons at the edges of the main dock area,
//      * a translucent preview rectangle of the resulting docked area.
//
//  Mouse clicks pass through (setInterceptsMouseClicks(false,false)) so the
//  user can still interact with whatever is behind.  Hit testing is driven
//  by the DockManager which feeds it the current cursor position.
//============================================================================
#pragma once

#include "DockTypes.h"

namespace docking
{

class DockOverlay : public juce::Component
{
public:
    struct GuideButton
    {
        DockSide              side    = DockSide::None;
        juce::Rectangle<int>  rect;
        bool                  isOuter = false;
    };

    DockOverlay (const DockTheme& t) : theme (t)
    {
        setInterceptsMouseClicks (false, false);
        setOpaque (false);
        setAlwaysOnTop (true);
        setVisible (false);
    }

    ~DockOverlay() override
    {
        if (isOnDesktop())
            removeFromDesktop();
    }

    //------------------------------------------------------------------------
    /** Reveals the overlay at the given screen rectangle as an always-on-top
        transparent desktop window. */
    void showOver (juce::Rectangle<int> screenArea)
    {
        if (! isOnDesktop())
            addToDesktop (juce::ComponentPeer::windowIgnoresMouseClicks
                        | juce::ComponentPeer::windowIsTemporary);

        setBounds (screenArea);
        setVisible (true);
        toFront (false);
    }

    /** Hides the overlay and clears any guides. */
    void hide()
    {
        clear();
        setVisible (false);
    }

    //------------------------------------------------------------------------
    void clear()
    {
        guides.clear();
        preview = {};
        hoverSide = DockSide::None;
        hoverOuter = false;
        repaint();
    }

    void addInnerGuides (juce::Point<int> centreLocal)
    {
        const int s = theme.guideSize;
        const int g = theme.guideGap;
        addGuide (DockSide::Center, { centreLocal.x - s/2, centreLocal.y - s/2, s, s }, false);
        addGuide (DockSide::Left,   { centreLocal.x - s/2 - s - g, centreLocal.y - s/2, s, s }, false);
        addGuide (DockSide::Right,  { centreLocal.x - s/2 + s + g, centreLocal.y - s/2, s, s }, false);
        addGuide (DockSide::Top,    { centreLocal.x - s/2, centreLocal.y - s/2 - s - g, s, s }, false);
        addGuide (DockSide::Bottom, { centreLocal.x - s/2, centreLocal.y - s/2 + s + g, s, s }, false);
        repaint();
    }

    void addOuterGuides (juce::Rectangle<int> areaLocal)
    {
        const int s = theme.guideSize;
        const int m = 14;
        addGuide (DockSide::Left,   { areaLocal.getX() + m,             areaLocal.getCentreY() - s/2, s, s }, true);
        addGuide (DockSide::Right,  { areaLocal.getRight() - m - s,     areaLocal.getCentreY() - s/2, s, s }, true);
        addGuide (DockSide::Top,    { areaLocal.getCentreX() - s/2,     areaLocal.getY() + m,         s, s }, true);
        addGuide (DockSide::Bottom, { areaLocal.getCentreX() - s/2,     areaLocal.getBottom() - m - s, s, s }, true);
        repaint();
    }

    void setPreview (juce::Rectangle<int> r)
    {
        if (preview != r)
        {
            preview = r;
            repaint();
        }
    }

    void setHoverSide (DockSide s, bool outer)
    {
        if (hoverSide != s || hoverOuter != outer)
        {
            hoverSide = s;
            hoverOuter = outer;
            repaint();
        }
    }

    DockSide getHoverSide()   const noexcept { return hoverSide;  }
    bool     isHoverOuter()   const noexcept { return hoverOuter; }

    //------------------------------------------------------------------------
    /** Hit-tests the cursor (in this component's local coords) against the
        guide buttons.  Returns the side and whether it is an outer guide. */
    std::pair<DockSide, bool> hitTest (juce::Point<int> p) const
    {
        // Outer guides take priority over inner if they overlap
        for (const auto& gb : guides)
            if (gb.isOuter && gb.rect.contains (p)) return { gb.side, true };
        for (const auto& gb : guides)
            if (! gb.isOuter && gb.rect.contains (p)) return { gb.side, false };
        return { DockSide::None, false };
    }

    //------------------------------------------------------------------------
    void paint (juce::Graphics& g) override
    {
        if (! preview.isEmpty())
        {
            g.setColour (theme.overlayHint);
            g.fillRect (preview);
            g.setColour (theme.overlayHintBorder);
            g.drawRect (preview, 2);
        }

        for (const auto& gb : guides)
            paintGuide (g, gb);
    }

private:
    void addGuide (DockSide side, juce::Rectangle<int> r, bool outer)
    {
        guides.push_back ({ side, r, outer });
    }

    void paintGuide (juce::Graphics& g, const GuideButton& gb) const
    {
        const bool isHover = (gb.side == hoverSide && gb.isOuter == hoverOuter && gb.side != DockSide::None);
        auto r = gb.rect.toFloat();

        // soft shadow for a tactile feel
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.fillRoundedRectangle (r.translated (0.0f, 1.0f).expanded (1.0f, 1.0f), 4.0f);

        g.setColour (isHover ? theme.guideHover : theme.guideFill);
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (theme.guideBorder);
        g.drawRoundedRectangle (r, 4.0f, 1.2f);

        // glyph
        const float cx = r.getCentreX();
        const float cy = r.getCentreY();
        const float h  = r.getHeight();
        const float pad = h * 0.18f;
        const float inset = h * 0.30f;

        juce::Path icon;
        switch (gb.side)
        {
            case DockSide::Center:
                icon.addRectangle (r.reduced (pad));
                icon.addRectangle (r.getX() + pad, r.getY() + pad, r.getWidth() - 2 * pad, h * 0.20f);
                break;
            case DockSide::Left:
                icon.addRectangle (r.getX() + pad, r.getY() + pad, h * 0.32f, r.getHeight() - 2 * pad);
                icon.addRectangle (r.getX() + pad + inset, r.getY() + pad, r.getWidth() - 2 * pad - inset, r.getHeight() - 2 * pad);
                break;
            case DockSide::Right:
                icon.addRectangle (r.getRight() - pad - h * 0.32f, r.getY() + pad, h * 0.32f, r.getHeight() - 2 * pad);
                icon.addRectangle (r.getX() + pad, r.getY() + pad, r.getWidth() - 2 * pad - inset, r.getHeight() - 2 * pad);
                break;
            case DockSide::Top:
                icon.addRectangle (r.getX() + pad, r.getY() + pad, r.getWidth() - 2 * pad, h * 0.32f);
                icon.addRectangle (r.getX() + pad, r.getY() + pad + inset, r.getWidth() - 2 * pad, r.getHeight() - 2 * pad - inset);
                break;
            case DockSide::Bottom:
                icon.addRectangle (r.getX() + pad, r.getBottom() - pad - h * 0.32f, r.getWidth() - 2 * pad, h * 0.32f);
                icon.addRectangle (r.getX() + pad, r.getY() + pad, r.getWidth() - 2 * pad, r.getHeight() - 2 * pad - inset);
                break;
            default:
                break;
        }

        g.setColour (isHover ? juce::Colours::white : theme.guideBorder.withAlpha (0.85f));
        g.fillPath (icon);
        g.setColour (theme.guideBorder);
        g.strokePath (icon, juce::PathStrokeType (1.0f));

        juce::ignoreUnused (cx, cy);
    }

    const DockTheme& theme;
    std::vector<GuideButton> guides;
    juce::Rectangle<int> preview;
    DockSide hoverSide = DockSide::None;
    bool     hoverOuter = false;
};

} // namespace docking
