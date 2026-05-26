//============================================================================
//  DockNode.h
//  The layout tree:
//      DockNode (abstract)
//          DockTabNode   - leaf, hosts N panels as tabs
//          DockSplitNode - holds N child nodes separated by splitters
//
//  Methods that reference DockManager are only DECLARED here and DEFINED
//  in DockingFramework.h (where DockManager is fully visible).
//============================================================================
#pragma once

#include "DockTypes.h"
#include "DockablePanel.h"

namespace docking
{

//============================================================================
//  DockTabBar - the row of tabs at the top of a DockTabNode.
//  Self-contained: depends only on DockTheme.
//============================================================================
class DockTabBar : public juce::Component
{
public:
    std::function<void (int)>                            onTabClicked;
    std::function<void (int, const juce::MouseEvent&)>   onTabDragStart;
    std::function<void (int)>                            onTabClose;

    DockTabBar (const DockTheme& t) : theme (t) { setWantsKeyboardFocus (false); }

    void setTabs (std::vector<juce::String> newNames, int newActive)
    {
        names = std::move (newNames);
        // Standard: alles schliessbar. Aufrufer, die einzelne Tabs ohne X
        // anzeigen wollen, rufen die Overload mit Flags-Vektor.
        closable.assign (names.size(), true);
        active = newActive;
        repaint();
    }

    void setTabs (std::vector<juce::String> newNames,
                  std::vector<bool> newClosable,
                  int newActive)
    {
        names = std::move (newNames);
        closable = std::move (newClosable);
        if (closable.size() != names.size())
            closable.assign (names.size(), true);
        active = newActive;
        repaint();
    }

    int  getActiveIndex() const noexcept { return active; }
    int  getNumTabs()     const noexcept { return (int) names.size(); }

    //------------------------------------------------------------------------
    void paint (juce::Graphics& g) override
    {
        g.fillAll (theme.background);

        const float h = (float) getHeight();
        int x = 0;

        for (int i = 0; i < (int) names.size(); ++i)
        {
            const int w = getTabWidth (i);
            const juce::Rectangle<float> r ((float) x, 0.0f, (float) w, h);
            const bool isActive = (i == active);
            const bool isHover  = (i == hoverTab);

            g.setColour (isActive ? theme.tabActive
                                  : isHover ? theme.tabHover : theme.tabInactive);
            g.fillRect (r);

            // bottom highlight on active tab
            if (isActive)
            {
                g.setColour (theme.headerActive.brighter (0.2f));
                g.fillRect (juce::Rectangle<float> (r.getX(), 0.0f, r.getWidth(), 2.0f));
            }

            g.setColour (theme.border);
            g.drawLine (r.getRight(), 0.0f, r.getRight(), h, 1.0f);

            g.setColour (theme.tabText);
            g.setFont (juce::Font (juce::FontOptions (h * 0.55f)));
            // Tab-Reserve fuer X nur, wenn dieser Tab schliessbar ist —
            // sonst kommt der Text bis an die rechte Kante.
            const bool isClosable = isTabClosable (i);
            auto textArea = r.reduced (8.0f, 0.0f).withTrimmedRight (isClosable ? 18.0f : 0.0f);
            g.drawText (names[(size_t) i], textArea, juce::Justification::centredLeft, true);

            if (isClosable && (isActive || isHover))
            {
                const auto cb = getCloseButtonBounds (i).toFloat();
                if (closeHoverTab == i)
                {
                    g.setColour (juce::Colour (0x66ffffff));
                    g.fillRoundedRectangle (cb.expanded (1.5f), 2.0f);
                }
                g.setColour (theme.tabText);
                g.drawLine (cb.getX() + 3, cb.getY() + 3, cb.getRight() - 3, cb.getBottom() - 3, 1.3f);
                g.drawLine (cb.getRight() - 3, cb.getY() + 3, cb.getX() + 3, cb.getBottom() - 3, 1.3f);
            }

            x += w;
        }

        // remaining strip
        if (x < getWidth())
        {
            g.setColour (theme.headerInactive);
            g.fillRect (juce::Rectangle<int> (x, 0, getWidth() - x, getHeight()));
        }
        g.setColour (theme.border);
        g.drawLine (0.0f, h - 0.5f, (float) getWidth(), h - 0.5f, 1.0f);
    }

    //------------------------------------------------------------------------
    void mouseMove (const juce::MouseEvent& e) override
    {
        const int t = hitTestTab (e.getPosition());
        const int c = hitTestClose (e.getPosition());
        if (t != hoverTab || c != closeHoverTab)
        {
            hoverTab = t;
            closeHoverTab = c;
            repaint();
        }
    }

    void mouseExit (const juce::MouseEvent&) override
    {
        if (hoverTab != -1 || closeHoverTab != -1)
        {
            hoverTab = closeHoverTab = -1;
            repaint();
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        pressedTab    = hitTestTab (e.getPosition());
        pressedClose  = hitTestClose (e.getPosition());
        dragStartPos  = e.getPosition();
        dragTriggered = false;

        if (pressedTab != -1 && pressedClose == -1)
        {
            if (active != pressedTab && onTabClicked)
                onTabClicked (pressedTab);
        }
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragTriggered || pressedTab == -1 || pressedClose != -1)
            return;

        if (e.getPosition().getDistanceFrom (dragStartPos) > 6)
        {
            dragTriggered = true;
            if (onTabDragStart)
                onTabDragStart (pressedTab, e);
        }
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (pressedClose != -1 && pressedClose == hitTestClose (e.getPosition()))
            if (onTabClose)
                onTabClose (pressedClose);

        pressedTab = pressedClose = -1;
        dragTriggered = false;
    }

    //------------------------------------------------------------------------
    int hitTestTab (juce::Point<int> p) const
    {
        if (p.getY() < 0 || p.getY() >= getHeight()) return -1;
        int x = 0;
        for (int i = 0; i < (int) names.size(); ++i)
        {
            const int w = getTabWidth (i);
            if (p.getX() >= x && p.getX() < x + w)
                return i;
            x += w;
        }
        return -1;
    }

    int hitTestClose (juce::Point<int> p) const
    {
        const int idx = hitTestTab (p);
        if (idx == -1) return -1;
        if (! isTabClosable (idx)) return -1;
        return getCloseButtonBounds (idx).contains (p) ? idx : -1;
    }

    bool isTabClosable (int i) const noexcept
    {
        return (i >= 0 && i < (int) closable.size()) ? closable[(size_t) i] : true;
    }

    juce::Rectangle<int> getTabBounds (int i) const
    {
        int x = 0;
        for (int k = 0; k < i; ++k) x += getTabWidth (k);
        return { x, 0, getTabWidth (i), getHeight() };
    }

    juce::Rectangle<int> getCloseButtonBounds (int i) const
    {
        const auto r = getTabBounds (i);
        const int s = juce::jmax (10, (int) (r.getHeight() * 0.55f));
        return { r.getRight() - s - 6, r.getY() + (r.getHeight() - s) / 2, s, s };
    }

private:
    const DockTheme& theme;
    std::vector<juce::String> names;
    std::vector<bool>         closable;
    int active        = -1;
    int hoverTab      = -1;
    int closeHoverTab = -1;
    int pressedTab    = -1;
    int pressedClose  = -1;
    juce::Point<int> dragStartPos;
    bool dragTriggered = false;

    int getTabWidth (int i) const
    {
        if (i < 0 || i >= (int) names.size()) return 0;
        // Coarse estimate – avoids JUCE 7/8 API drift around Font::getStringWidth.
        const float fontH = (float) getHeight() * 0.55f;
        const int   textW = (int) std::ceil (names[(size_t) i].length() * fontH * 0.55f);
        return juce::jmax (80, textW + 42);
    }
};

//============================================================================
//  DockSplitter - a thin draggable bar between two children of a SplitNode.
//============================================================================
class DockSplitter : public juce::Component
{
public:
    std::function<void (int)> onDragged;
    std::function<void()>     onDragEnded;

    DockSplitter (const DockTheme& t, SplitOrientation o)
        : theme (t), orientation (o)
    {
        setMouseCursor (orientation == SplitOrientation::Horizontal
                          ? juce::MouseCursor::LeftRightResizeCursor
                          : juce::MouseCursor::UpDownResizeCursor);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll ((hover || dragging) ? theme.splitterHover : theme.splitter);
    }

    void mouseEnter (const juce::MouseEvent&) override { hover = true;  repaint(); }
    void mouseExit  (const juce::MouseEvent&) override { hover = false; repaint(); }

    void mouseDown (const juce::MouseEvent& e) override
    {
        dragging = true;
        lastPos = (orientation == SplitOrientation::Horizontal) ? e.getScreenX() : e.getScreenY();
        repaint();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (! dragging) return;
        const int cur = (orientation == SplitOrientation::Horizontal) ? e.getScreenX() : e.getScreenY();
        const int delta = cur - lastPos;
        if (delta != 0 && onDragged)
        {
            onDragged (delta);
            lastPos = cur;
        }
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        dragging = false;
        repaint();
        if (onDragEnded) onDragEnded();
    }

private:
    const DockTheme& theme;
    SplitOrientation orientation;
    bool hover = false;
    bool dragging = false;
    int  lastPos  = 0;
};

//============================================================================
//  DockNode - abstract base class for the tree.
//============================================================================
class DockNode : public juce::Component
{
public:
    DockNode (DockManager& m) : manager (m) {}
    ~DockNode() override = default;

    DockManager& getManager() const noexcept { return manager; }

    virtual juce::ValueTree saveState() const = 0;
    virtual bool isEmpty() const = 0;
    virtual void collectTabNodes (std::vector<DockTabNode*>& out) = 0;

protected:
    DockManager& manager;
};

//============================================================================
//  DockTabNode - a leaf node hosting one or more panels as tabs.
//============================================================================
class DockTabNode : public DockNode
{
public:
    DockTabNode (DockManager& m);                              // defined later

    void addPanel (DockablePanel* p, int index = -1);          // defined later
    void removePanel (DockablePanel* p);                       // defined later

    int             getNumPanels()      const noexcept { return (int) panels.size(); }
    DockablePanel*  getPanel (int i)    const          { return panels[(size_t) i]; }
    int             getActiveIndex()    const noexcept { return activeIndex; }
    DockablePanel*  getActivePanel()    const
    {
        return (activeIndex >= 0 && activeIndex < (int) panels.size())
                  ? panels[(size_t) activeIndex] : nullptr;
    }
    void  setActiveIndex (int idx);                            // defined later

    /** Re-fetch the tab strip from the current panel set. Public hook for
        callers that mutate a property of a panel that influences the tab
        rendering (e.g. setPanelName, setClosable). */
    void refreshTabsExternal() { refreshTabs(); repaint(); }
    int   indexOf (const DockablePanel* p) const
    {
        for (int i = 0; i < (int) panels.size(); ++i)
            if (panels[(size_t) i] == p) return i;
        return -1;
    }

    const std::vector<DockablePanel*>& getPanels() const noexcept { return panels; }

    //----- DockNode --------------------------------------------------------
    juce::ValueTree saveState() const override;                // defined later
    bool isEmpty() const override { return panels.empty(); }
    void collectTabNodes (std::vector<DockTabNode*>& out) override { out.push_back (this); }

    //----- Component -------------------------------------------------------
    void paint (juce::Graphics& g) override;                   // defined later
    void resized() override;                                   // defined later

    //----- Drop target hit testing -----------------------------------------
    /** Side relative to this node's centre for the inner dock guide.
        localPoint must be in this component's coordinate system. */
    DockSide hitInnerGuide (juce::Point<int> localPoint) const;

    /** Rect of an inner guide button in local coords. */
    juce::Rectangle<int> getInnerGuideRect (DockSide side) const;

    /** Tab bar bounds in local coords (used by overlay for tabify hint). */
    juce::Rectangle<int> getTabBarBounds() const { return tabBar ? tabBar->getBounds() : juce::Rectangle<int>(); }

private:
    std::unique_ptr<DockTabBar> tabBar;
    std::vector<DockablePanel*> panels;          // non-owning; DockManager owns
    int activeIndex = -1;

    void refreshTabs();
    void onTabClick (int i);
    void onTabDragStart (int i, const juce::MouseEvent& e);
    void onTabClose (int i);
};

//============================================================================
//  DockSplitNode - a container split horizontally or vertically.
//============================================================================
class DockSplitNode : public DockNode
{
public:
    DockSplitNode (DockManager& m, SplitOrientation o) : DockNode (m), orientation (o) {}

    SplitOrientation  getOrientation() const noexcept { return orientation; }
    int               getNumChildren() const noexcept { return (int) children.size(); }
    DockNode*         getChild (int i) const          { return children[(size_t) i].get(); }
    int               getChildSize (int i) const      { return sizes[(size_t) i]; }
    int               indexOf (const DockNode* n) const
    {
        for (int i = 0; i < (int) children.size(); ++i)
            if (children[(size_t) i].get() == n) return i;
        return -1;
    }

    void   addChild    (std::unique_ptr<DockNode> child, int sizePx = 0, int atIndex = -1);
    std::unique_ptr<DockNode> removeChild (int idx);

    /** Override the recorded size for one child along the split axis. The
        next resized() call will renormalise all sizes to the available
        space, so callers can pass raw target pixels; the helper just makes
        the relative weight stick when the parent is laid out. */
    void   setChildSize (int idx, int sizePx)
    {
        if (idx < 0 || idx >= (int) sizes.size()) return;
        sizes[(size_t) idx] = juce::jmax (0, sizePx);
    }

    /** Changes the split orientation in-place.  Cursors of existing
        splitters are updated; children keep their relative ordering. */
    void   setOrientation (SplitOrientation newOrientation);

    //----- DockNode --------------------------------------------------------
    juce::ValueTree saveState() const override;
    bool isEmpty() const override
    {
        for (auto& c : children) if (! c->isEmpty()) return false;
        return true;
    }
    void collectTabNodes (std::vector<DockTabNode*>& out) override
    {
        for (auto& c : children) c->collectTabNodes (out);
    }

    //----- Component -------------------------------------------------------
    void resized() override;

private:
    SplitOrientation orientation;
    std::vector<std::unique_ptr<DockNode>>     children;
    std::vector<int>                           sizes;      // sizes along orientation axis (px)
    std::vector<std::unique_ptr<DockSplitter>> splitters;

    void rebuildSplitters();
    void onSplitterDrag (int splitterIndex, int delta);
};

} // namespace docking
