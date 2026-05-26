//============================================================================
//  FloatingDockWindow.h
//  A top-level desktop window hosting one DockTabNode.
//  Custom title bar (drag = move + drive the manager's drop overlay),
//  a close button and a resize corner.
//
//  Most non-trivial methods are declared here and defined in
//  DockingFramework.h, since they need the full DockManager interface.
//============================================================================
#pragma once

#include "DockTypes.h"
#include "DockNode.h"

namespace docking
{

class FloatingDockWindow : public juce::Component
{
public:
    explicit FloatingDockWindow (DockManager& m);
    ~FloatingDockWindow() override;

    void setTabNode (std::unique_ptr<DockTabNode> n);
    DockTabNode*               getTabNode() const noexcept { return tabNode.get(); }
    std::unique_ptr<DockTabNode> releaseTabNode();

    /** Sets the initial cursor anchor so that the first drag motion moves
        the window predictably (used when this window was just born from a
        drag-undock). */
    void seedDragState (juce::Point<int> screenAnchor);

    juce::String getTitle() const;

    //----- Component -------------------------------------------------------
    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown   (const juce::MouseEvent& e) override;
    void mouseDrag   (const juce::MouseEvent& e) override;
    void mouseUp     (const juce::MouseEvent& e) override;
    void mouseMove   (const juce::MouseEvent& e) override;
    void mouseExit   (const juce::MouseEvent& e) override;
    juce::MouseCursor getMouseCursor() override;

    //----- Layout metrics --------------------------------------------------
    int  getTitleHeight() const noexcept { return titleHeight; }
    int  getBorderSize()  const noexcept { return borderSize;  }

    juce::Rectangle<int> titleRect() const   { return { 0, 0, getWidth(), titleHeight }; }
    juce::Rectangle<int> closeRect() const
    {
        const int s = juce::jmax (12, (int) (titleHeight * 0.60f));
        return { getWidth() - s - 8, (titleHeight - s) / 2, s, s };
    }
    juce::Rectangle<int> resizeBRRect() const
    {
        const int s = juce::jmax (8, borderSize * 3);
        return { getWidth() - s, getHeight() - s, s, s };
    }
    juce::Rectangle<int> contentRect() const
    {
        return { 0, titleHeight, getWidth(), juce::jmax (0, getHeight() - titleHeight) };
    }

private:
    enum class Region { None, Title, Close, ResizeBR, Content };

    DockManager& manager;
    std::unique_ptr<DockTabNode> tabNode;

    int  titleHeight = 24;
    int  borderSize  = 5;

    Region                pressedRegion = Region::None;
    bool                  dragInProgress = false;
    bool                  closeHover = false;
    juce::Point<int>      dragStartScreen;
    juce::Rectangle<int>  dragStartBounds;

    Region hitRegion (juce::Point<int> p) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FloatingDockWindow)
};

} // namespace docking
