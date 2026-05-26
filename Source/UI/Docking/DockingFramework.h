//============================================================================
//  DockingFramework.h
//  Umbrella header.  Include this single file in your translation units to
//  use the docking framework.  Defines all the methods that need a fully
//  visible DockManager (and therefore could not live in the per-class
//  headers).  Everything is marked inline so the framework remains
//  header-only.
//============================================================================
#pragma once

#include "DockTypes.h"
#include "DockablePanel.h"
#include "DockNode.h"
#include "DockOverlay.h"
#include "FloatingDockWindow.h"
#include "DockManager.h"

namespace docking
{

//============================================================================
//  DockablePanel - late-bound members
//============================================================================
inline const DockTheme& DockablePanel::getTheme() const noexcept
{
    static const DockTheme fallback;
    return manager != nullptr ? manager->getTheme() : fallback;
}

inline void DockablePanel::notifyTabRefresh()
{
    // Wandere die Component-Hierarchie hoch bis zum naechsten DockTabNode
    // (die Group-Components zwischen Panel und Tab-Node sind keine DockNode-
    // Subklassen). Wenn gefunden, dessen Tab-Strip neu aufbauen.
    juce::Component* c = getParentComponent();
    while (c != nullptr)
    {
        if (auto* tn = dynamic_cast<DockTabNode*> (c))
        {
            tn->refreshTabsExternal();
            return;
        }
        c = c->getParentComponent();
    }
}

//============================================================================
//  DockTabNode
//============================================================================
inline DockTabNode::DockTabNode (DockManager& m) : DockNode (m)
{
    tabBar = std::make_unique<DockTabBar> (m.getTheme());
    tabBar->onTabClicked   = [this] (int i)                              { onTabClick (i); };
    tabBar->onTabDragStart = [this] (int i, const juce::MouseEvent& e)   { onTabDragStart (i, e); };
    tabBar->onTabClose     = [this] (int i)                              { onTabClose (i); };
    addAndMakeVisible (*tabBar);
}

inline void DockTabNode::addPanel (DockablePanel* p, int index)
{
    if (p == nullptr) return;

    // Detach from any previous parent (re-parenting is intentional).
    if (auto* parent = p->getParentComponent())
        parent->removeChildComponent (p);

    if (index < 0 || index > (int) panels.size())
        index = (int) panels.size();

    panels.insert (panels.begin() + index, p);
    addChildComponent (*p);
    setActiveIndex (index);
}

inline void DockTabNode::removePanel (DockablePanel* p)
{
    const auto it = std::find (panels.begin(), panels.end(), p);
    if (it == panels.end()) return;

    removeChildComponent (p);
    const int removedIndex = (int) std::distance (panels.begin(), it);
    panels.erase (it);

    if (panels.empty())
        activeIndex = -1;
    else if (activeIndex >= (int) panels.size())
        activeIndex = (int) panels.size() - 1;
    else if (activeIndex > removedIndex)
        --activeIndex;

    setActiveIndex (activeIndex);
}

inline void DockTabNode::setActiveIndex (int idx)
{
    activeIndex = juce::jlimit (-1, (int) panels.size() - 1, idx);

    for (int i = 0; i < (int) panels.size(); ++i)
        panels[(size_t) i]->setVisible (i == activeIndex);

    refreshTabs();
    resized();
    repaint();
}

inline juce::ValueTree DockTabNode::saveState() const
{
    juce::ValueTree v (ids::Tabs);
    v.setProperty (ids::ActiveIndex, activeIndex, nullptr);
    for (auto* p : panels)
    {
        juce::ValueTree pv (ids::Panel);
        pv.setProperty (ids::PanelId, p->getPanelId(), nullptr);
        v.appendChild (pv, nullptr);
    }
    return v;
}

inline void DockTabNode::paint (juce::Graphics& g)
{
    g.fillAll (manager.getTheme().background);
    g.setColour (manager.getTheme().border);
    g.drawRect (getLocalBounds(), 1);
}

inline void DockTabNode::resized()
{
    auto r = getLocalBounds().reduced (1);
    const int tabH = manager.getTheme().tabHeight;
    if (tabBar) tabBar->setBounds (r.removeFromTop (tabH));

    for (auto* p : panels)
        if (p == getActivePanel())
            p->setBounds (r);
}

inline void DockTabNode::refreshTabs()
{
    std::vector<juce::String> names;
    std::vector<bool>         closable;
    names.reserve (panels.size());
    closable.reserve (panels.size());
    for (auto* p : panels)
    {
        names.push_back (p->getPanelName());
        closable.push_back (p->isClosable());
    }
    if (tabBar) tabBar->setTabs (std::move (names), std::move (closable), activeIndex);
}

inline void DockTabNode::onTabClick (int i)        { setActiveIndex (i); }

inline void DockTabNode::onTabDragStart (int i, const juce::MouseEvent& e)
{
    if (i < 0 || i >= (int) panels.size()) return;
    manager.beginTabDrag (panels[(size_t) i], e);
}

inline void DockTabNode::onTabClose (int i)
{
    if (i < 0 || i >= (int) panels.size()) return;
    auto* p = panels[(size_t) i];
    // Defense in depth: das X wird fuer nicht-schliessbare Panels gar nicht
    // mehr gerendert, aber falls ein anderer Pfad onTabClose direkt aufruft,
    // hier nochmal absichern.
    if (p == nullptr || ! p->isClosable()) return;
    manager.requestClosePanel (p);
}

inline DockSide DockTabNode::hitInnerGuide (juce::Point<int> p) const
{
    const int s = manager.getTheme().guideSize;
    const int g = manager.getTheme().guideGap;
    const auto c = getLocalBounds().getCentre();
    const juce::Rectangle<int> centre (c.x - s/2, c.y - s/2, s, s);
    const juce::Rectangle<int> left   (c.x - s/2 - s - g, c.y - s/2, s, s);
    const juce::Rectangle<int> right  (c.x - s/2 + s + g, c.y - s/2, s, s);
    const juce::Rectangle<int> top    (c.x - s/2, c.y - s/2 - s - g, s, s);
    const juce::Rectangle<int> bottom (c.x - s/2, c.y - s/2 + s + g, s, s);
    if (centre.contains (p)) return DockSide::Center;
    if (left.contains   (p)) return DockSide::Left;
    if (right.contains  (p)) return DockSide::Right;
    if (top.contains    (p)) return DockSide::Top;
    if (bottom.contains (p)) return DockSide::Bottom;
    return DockSide::None;
}

inline juce::Rectangle<int> DockTabNode::getInnerGuideRect (DockSide side) const
{
    const int s = manager.getTheme().guideSize;
    const int g = manager.getTheme().guideGap;
    const auto c = getLocalBounds().getCentre();
    switch (side)
    {
        case DockSide::Center: return { c.x - s/2,         c.y - s/2,         s, s };
        case DockSide::Left:   return { c.x - s/2 - s - g, c.y - s/2,         s, s };
        case DockSide::Right:  return { c.x - s/2 + s + g, c.y - s/2,         s, s };
        case DockSide::Top:    return { c.x - s/2,         c.y - s/2 - s - g, s, s };
        case DockSide::Bottom: return { c.x - s/2,         c.y - s/2 + s + g, s, s };
        default: return {};
    }
}

//============================================================================
//  DockSplitNode
//============================================================================
inline void DockSplitNode::addChild (std::unique_ptr<DockNode> child, int sizePx, int atIndex)
{
    if (child == nullptr) return;
    if (atIndex < 0 || atIndex > (int) children.size())
        atIndex = (int) children.size();

    addAndMakeVisible (*child);
    children.insert (children.begin() + atIndex, std::move (child));
    sizes.insert (sizes.begin() + atIndex, sizePx);
    rebuildSplitters();
    resized();
}

inline std::unique_ptr<DockNode> DockSplitNode::removeChild (int idx)
{
    if (idx < 0 || idx >= (int) children.size()) return nullptr;

    auto n = std::move (children[(size_t) idx]);
    children.erase (children.begin() + idx);
    sizes.erase (sizes.begin() + idx);
    if (n) removeChildComponent (n.get());
    rebuildSplitters();
    resized();
    return n;
}

inline void DockSplitNode::setOrientation (SplitOrientation newOrientation)
{
    if (orientation == newOrientation) return;
    orientation = newOrientation;
    // Recreate splitters with the new cursor and re-layout.
    rebuildSplitters();
    resized();
}

inline void DockSplitNode::resized()
{
    if (children.empty()) return;

    auto r = getLocalBounds();
    const int splitterSize = manager.getTheme().splitterSize;
    const int splitterCount = juce::jmax (0, (int) children.size() - 1);
    const int available = (orientation == SplitOrientation::Horizontal) ? r.getWidth() : r.getHeight();
    const int contentAvail = juce::jmax (0, available - splitterSize * splitterCount);

    int sum = 0;
    for (auto s : sizes) sum += juce::jmax (0, s);

    if (sum <= 0)
    {
        const int each = contentAvail / juce::jmax (1, (int) children.size());
        for (auto& s : sizes) s = each;
        sum = each * (int) sizes.size();
    }

    if (sum != contentAvail && sum > 0)
    {
        const double scale = (double) contentAvail / (double) sum;
        int leftover = contentAvail;
        for (size_t i = 0; i + 1 < sizes.size(); ++i)
        {
            sizes[i] = juce::jmax (40, (int) std::round (sizes[i] * scale));
            leftover -= sizes[i];
        }
        if (! sizes.empty())
            sizes.back() = juce::jmax (40, leftover);
    }

    int pos = (orientation == SplitOrientation::Horizontal) ? r.getX() : r.getY();
    for (size_t i = 0; i < children.size(); ++i)
    {
        const int sz = sizes[i];
        if (orientation == SplitOrientation::Horizontal)
            children[i]->setBounds (pos, r.getY(), sz, r.getHeight());
        else
            children[i]->setBounds (r.getX(), pos, r.getWidth(), sz);
        pos += sz;

        if (i < splitters.size())
        {
            if (orientation == SplitOrientation::Horizontal)
                splitters[i]->setBounds (pos, r.getY(), splitterSize, r.getHeight());
            else
                splitters[i]->setBounds (r.getX(), pos, r.getWidth(), splitterSize);
            pos += splitterSize;
        }
    }
}

inline juce::ValueTree DockSplitNode::saveState() const
{
    juce::ValueTree v (ids::Split);
    v.setProperty (ids::Orientation, juce::String (toString (orientation)), nullptr);

    juce::String szStr;
    for (size_t i = 0; i < sizes.size(); ++i)
    {
        if (i > 0) szStr += ",";
        szStr += juce::String (sizes[i]);
    }
    v.setProperty (ids::Sizes, szStr, nullptr);

    for (auto& c : children)
        v.appendChild (c->saveState(), nullptr);
    return v;
}

inline void DockSplitNode::rebuildSplitters()
{
    splitters.clear();
    const int n = (int) children.size();
    for (int i = 0; i < n - 1; ++i)
    {
        auto s = std::make_unique<DockSplitter> (manager.getTheme(), orientation);
        const int idx = i;
        s->onDragged = [this, idx] (int delta) { onSplitterDrag (idx, delta); };
        addAndMakeVisible (*s);
        splitters.push_back (std::move (s));
    }
}

inline void DockSplitNode::onSplitterDrag (int idx, int delta)
{
    if (idx < 0 || idx + 1 >= (int) sizes.size()) return;
    const int minSize = 60;
    const int newA = sizes[(size_t) idx] + delta;
    const int newB = sizes[(size_t) idx + 1] - delta;
    if (newA < minSize || newB < minSize) return;
    sizes[(size_t) idx]     = newA;
    sizes[(size_t) idx + 1] = newB;
    resized();
}

//============================================================================
//  FloatingDockWindow
//============================================================================
inline FloatingDockWindow::FloatingDockWindow (DockManager& m) : manager (m)
{
    setOpaque (true);
    setSize (360, 240);
    addToDesktop (juce::ComponentPeer::windowHasDropShadow
                | juce::ComponentPeer::windowIsTemporary);
    setVisible (true);
}

inline FloatingDockWindow::~FloatingDockWindow()
{
    removeFromDesktop();
}

inline void FloatingDockWindow::setTabNode (std::unique_ptr<DockTabNode> n)
{
    if (tabNode)
        removeChildComponent (tabNode.get());
    tabNode = std::move (n);
    if (tabNode)
    {
        addAndMakeVisible (*tabNode);
        resized();
        repaint();
    }
}

inline std::unique_ptr<DockTabNode> FloatingDockWindow::releaseTabNode()
{
    if (tabNode)
        removeChildComponent (tabNode.get());
    auto r = std::move (tabNode);
    repaint();
    return r;
}

inline void FloatingDockWindow::seedDragState (juce::Point<int> screenAnchor)
{
    pressedRegion   = Region::Title;
    dragInProgress  = true;
    dragStartScreen = screenAnchor;
    dragStartBounds = getScreenBounds();
}

inline juce::String FloatingDockWindow::getTitle() const
{
    if (tabNode != nullptr)
    {
        if (tabNode->getNumPanels() == 1)
            if (auto* p = tabNode->getActivePanel())
                return p->getPanelName();
        if (tabNode->getNumPanels() > 1)
            return juce::String (tabNode->getNumPanels()) + " panels";
    }
    return {};
}

inline void FloatingDockWindow::paint (juce::Graphics& g)
{
    const auto& theme = manager.getTheme();
    g.fillAll (theme.background);

    const auto tr = titleRect();
    g.setColour (theme.headerActive);
    g.fillRect (tr);

    g.setColour (theme.headerText);
    g.setFont (juce::Font (juce::FontOptions ((float) tr.getHeight() * 0.55f)));
    auto txt = tr.reduced (10, 0).withTrimmedRight (closeRect().getWidth() + 14);
    g.drawText (getTitle(), txt, juce::Justification::centredLeft, true);

    const auto cb = closeRect();
    if (closeHover)
    {
        g.setColour (juce::Colour (0x66ffffff));
        g.fillRoundedRectangle (cb.toFloat().expanded (1.5f), 2.0f);
    }
    g.setColour (theme.headerText);
    g.drawLine ((float) cb.getX() + 3,    (float) cb.getY() + 3,
                (float) cb.getRight() - 3, (float) cb.getBottom() - 3, 1.5f);
    g.drawLine ((float) cb.getRight() - 3, (float) cb.getY() + 3,
                (float) cb.getX() + 3,     (float) cb.getBottom() - 3, 1.5f);

    g.setColour (theme.border);
    g.drawRect (getLocalBounds(), 1);

    // small resize grip in bottom-right corner
    g.setColour (theme.splitter);
    const auto rs = resizeBRRect();
    for (int i = 0; i < 3; ++i)
    {
        const int s = 2;
        g.fillRect (rs.getRight() - s - i * 4, rs.getBottom() - s - i * 4, s, s);
    }
}

inline void FloatingDockWindow::resized()
{
    if (tabNode) tabNode->setBounds (contentRect());
}

inline FloatingDockWindow::Region FloatingDockWindow::hitRegion (juce::Point<int> p) const
{
    if (closeRect().contains (p))     return Region::Close;
    if (resizeBRRect().contains (p))  return Region::ResizeBR;
    if (titleRect().contains (p))     return Region::Title;
    if (contentRect().contains (p))   return Region::Content;
    return Region::None;
}

inline void FloatingDockWindow::mouseDown (const juce::MouseEvent& e)
{
    pressedRegion   = hitRegion (e.getPosition());
    dragStartScreen = e.getScreenPosition();
    dragStartBounds = getScreenBounds();
    dragInProgress  = false;

    toFront (true);
}

inline void FloatingDockWindow::mouseDrag (const juce::MouseEvent& e)
{
    if (pressedRegion == Region::Title)
    {
        if (! dragInProgress && e.getDistanceFromDragStart() > 3)
        {
            dragInProgress = true;
            manager.beginFloatingDrag (this, e);
        }
        // After the manager takes over, all motion is driven by its global
        // mouse listener; we do nothing further here.
    }
    else if (pressedRegion == Region::ResizeBR)
    {
        const auto delta = e.getScreenPosition() - dragStartScreen;
        const int newW = juce::jmax (180, dragStartBounds.getWidth()  + delta.x);
        const int newH = juce::jmax (100, dragStartBounds.getHeight() + delta.y);
        setBounds (dragStartBounds.withSize (newW, newH));
    }
}

inline void FloatingDockWindow::mouseUp (const juce::MouseEvent& e)
{
    if (pressedRegion == Region::Close && hitRegion (e.getPosition()) == Region::Close)
        manager.closeFloatingWindow (this);

    pressedRegion  = Region::None;
    dragInProgress = false;
}

inline void FloatingDockWindow::mouseMove (const juce::MouseEvent& e)
{
    const bool nh = closeRect().contains (e.getPosition());
    if (nh != closeHover) { closeHover = nh; repaint(); }
}

inline void FloatingDockWindow::mouseExit (const juce::MouseEvent&)
{
    if (closeHover) { closeHover = false; repaint(); }
}

inline juce::MouseCursor FloatingDockWindow::getMouseCursor()
{
    const auto pos = getMouseXYRelative();
    if (resizeBRRect().contains (pos))
        return juce::MouseCursor::BottomRightCornerResizeCursor;
    return juce::MouseCursor::NormalCursor;
}

//============================================================================
//  DockManager
//============================================================================
inline DockManager::DockManager()
{
    setOpaque (true);
    rootNode = std::make_unique<DockSplitNode> (*this, SplitOrientation::Horizontal);
    addAndMakeVisible (*rootNode);
    overlay = std::make_unique<DockOverlay> (theme);
}

inline DockManager::~DockManager()
{
    if (dragMode != DragMode::None)
        juce::Desktop::getInstance().removeGlobalMouseListener (this);
    floatingWindows.clear();
    overlay.reset();
    rootNode.reset();
}

inline void DockManager::paint (juce::Graphics& g)
{
    g.fillAll (theme.background);
}

inline void DockManager::resized()
{
    if (rootNode) rootNode->setBounds (getLocalBounds());
}

inline void DockManager::repaintAll()
{
    repaint();
    for (auto& fw : floatingWindows)
        if (fw) fw->repaint();
}

inline DockablePanel* DockManager::createPanel (const juce::String& id,
                                                const juce::String& name,
                                                std::unique_ptr<juce::Component> content,
                                                DockSide initialSide)
{
    if (findPanel (id))
    {
        jassertfalse; // duplicate panel ID
        return nullptr;
    }
    auto* raw = new DockablePanel (id, name, std::move (content));
    raw->setDockManager (this);
    allPanels.add (raw);
    addPanelInitially (raw, initialSide);
    return raw;
}

inline DockablePanel* DockManager::findPanel (const juce::String& id) const
{
    for (auto* p : allPanels)
        if (p->getPanelId() == id) return p;
    return nullptr;
}

inline void DockManager::addPanelInitially (DockablePanel* p, DockSide side)
{
    if (rootNode->getNumChildren() == 0)
    {
        auto tab = std::make_unique<DockTabNode> (*this);
        tab->addPanel (p);
        rootNode->addChild (std::move (tab), 200);
        return;
    }

    if (side == DockSide::Center || side == DockSide::None)
    {
        if (auto* t = firstTabNodeOrCreate())
        {
            t->addPanel (p);
            return;
        }
    }

    dockPanelAtEdge (p, side);
}

inline DockTabNode* DockManager::firstTabNodeOrCreate()
{
    std::vector<DockTabNode*> nodes;
    rootNode->collectTabNodes (nodes);
    if (! nodes.empty()) return nodes.front();

    auto tab = std::make_unique<DockTabNode> (*this);
    auto* raw = tab.get();
    rootNode->addChild (std::move (tab), 200);
    return raw;
}

inline void DockManager::detachPanelFromLayout (DockablePanel* p)
{
    if (p == nullptr) return;

    std::vector<DockTabNode*> nodes;
    rootNode->collectTabNodes (nodes);
    for (auto* n : nodes)
    {
        if (n->indexOf (p) >= 0)
        {
            n->removePanel (p);
            return;
        }
    }
    for (auto& fw : floatingWindows)
    {
        auto* tn = fw->getTabNode();
        if (tn && tn->indexOf (p) >= 0)
        {
            tn->removePanel (p);
            return;
        }
    }
}

inline void DockManager::closePanel (DockablePanel* p)
{
    if (p == nullptr) return;
    detachPanelFromLayout (p);
    allPanels.removeObject (p, true);
    simplifyTree();
}

inline void DockManager::activatePanel (DockablePanel* p)
{
    if (p == nullptr) return;
    std::vector<DockTabNode*> nodes;
    rootNode->collectTabNodes (nodes);
    for (auto* n : nodes)
    {
        const int i = n->indexOf (p);
        if (i >= 0) { n->setActiveIndex (i); return; }
    }
    for (auto& fw : floatingWindows)
    {
        if (auto* tn = fw->getTabNode())
        {
            const int i = tn->indexOf (p);
            if (i >= 0) { tn->setActiveIndex (i); fw->toFront (true); return; }
        }
    }
}

inline void DockManager::ensureRootOrientation (SplitOrientation o)
{
    if (rootNode->getOrientation() == o) return;

    if (rootNode->getNumChildren() <= 1)
    {
        rootNode->setOrientation (o);
        return;
    }

    // Wrap the existing children in a sub-split with the OLD orientation,
    // then flip the root.
    auto wrap = std::make_unique<DockSplitNode> (*this, rootNode->getOrientation());
    while (rootNode->getNumChildren() > 0)
    {
        const int sz = rootNode->getChildSize (0);
        auto c = rootNode->removeChild (0);
        wrap->addChild (std::move (c), sz);
    }
    rootNode->setOrientation (o);
    rootNode->addChild (std::move (wrap), 200);
}

inline void DockManager::dockPanel (DockablePanel* p, DockTabNode* target, DockSide side)
{
    if (p == nullptr || target == nullptr) return;

    if (side == DockSide::Center)
    {
        detachPanelFromLayout (p);
        target->addPanel (p);
        simplifyTree();
        return;
    }

    detachPanelFromLayout (p);
    auto newTab = std::make_unique<DockTabNode> (*this);
    newTab->addPanel (p);
    dockTabsRelative (std::move (newTab), target, side);
}

inline void DockManager::dockTabsRelative (std::unique_ptr<DockTabNode> source,
                                           DockTabNode* target, DockSide side)
{
    if (source == nullptr || target == nullptr) return;

    if (side == DockSide::Center)
    {
        const auto pans = source->getPanels();
        for (auto* p : pans) target->addPanel (p);
        simplifyTree();
        return;
    }

    auto* parentSplit = dynamic_cast<DockSplitNode*> (target->getParentComponent());
    if (parentSplit == nullptr) { jassertfalse; return; }

    const SplitOrientation needed = (side == DockSide::Left || side == DockSide::Right)
                                        ? SplitOrientation::Horizontal
                                        : SplitOrientation::Vertical;
    const int idx = parentSplit->indexOf (target);

    if (parentSplit->getOrientation() == needed)
    {
        const int newIdx    = (side == DockSide::Left || side == DockSide::Top) ? idx : idx + 1;
        const int splitSize = juce::jmax (60, parentSplit->getChildSize (idx) / 2);
        parentSplit->addChild (std::move (source), splitSize, newIdx);
    }
    else
    {
        const int sz = parentSplit->getChildSize (idx);
        auto oldTarget = parentSplit->removeChild (idx);
        auto newSplit  = std::make_unique<DockSplitNode> (*this, needed);
        if (side == DockSide::Left || side == DockSide::Top)
        {
            newSplit->addChild (std::move (source),    100);
            newSplit->addChild (std::move (oldTarget), 100);
        }
        else
        {
            newSplit->addChild (std::move (oldTarget), 100);
            newSplit->addChild (std::move (source),    100);
        }
        parentSplit->addChild (std::move (newSplit), sz, idx);
    }
    simplifyTree();
}

inline void DockManager::dockPanelAtEdge (DockablePanel* p, DockSide side)
{
    if (p == nullptr || side == DockSide::None || side == DockSide::Center) return;
    detachPanelFromLayout (p);
    auto newTab = std::make_unique<DockTabNode> (*this);
    newTab->addPanel (p);
    dockTabsAtEdge (std::move (newTab), side);
}

inline void DockManager::dockTabsAtEdge (std::unique_ptr<DockTabNode> source, DockSide side)
{
    if (source == nullptr) return;
    const SplitOrientation needed = (side == DockSide::Left || side == DockSide::Right)
                                        ? SplitOrientation::Horizontal
                                        : SplitOrientation::Vertical;
    ensureRootOrientation (needed);
    const int insertAt = (side == DockSide::Left || side == DockSide::Top) ? 0 : -1;
    rootNode->addChild (std::move (source), 220, insertAt);
    simplifyTree();
}

inline FloatingDockWindow* DockManager::floatPanel (DockablePanel* p,
                                                    juce::Point<int> screenPos,
                                                    juce::Point<int> size)
{
    if (p == nullptr) return nullptr;
    detachPanelFromLayout (p);

    auto tab = std::make_unique<DockTabNode> (*this);
    tab->addPanel (p);
    auto fw = std::make_unique<FloatingDockWindow> (*this);
    fw->setTabNode (std::move (tab));
    fw->setBounds (screenPos.x, screenPos.y, size.x, size.y);
    auto* raw = fw.get();
    floatingWindows.push_back (std::move (fw));
    simplifyTree();
    return raw;
}

inline void DockManager::closeFloatingWindow (FloatingDockWindow* fw)
{
    if (fw == nullptr) return;

    // Collect panels in this window so we can delete them from allPanels.
    std::vector<DockablePanel*> toDelete;
    if (auto* tn = fw->getTabNode())
    {
        for (int i = 0; i < tn->getNumPanels(); ++i)
            toDelete.push_back (tn->getPanel (i));
    }

    for (auto it = floatingWindows.begin(); it != floatingWindows.end(); ++it)
    {
        if (it->get() == fw)
        {
            floatingWindows.erase (it);
            break;
        }
    }

    for (auto* p : toDelete)
        allPanels.removeObject (p, true);
}

//----- simplifyTree ---------------------------------------------------------

inline void DockManager::simplifyTree()
{
    bool changed = true;
    int  safety  = 64;
    while (changed && safety-- > 0)
    {
        changed = false;

        // 1. Drop empty tab nodes anywhere under the root.
        std::function<void (DockNode*)> dropEmptyTabs;
        dropEmptyTabs = [&] (DockNode* n)
        {
            if (auto* sp = dynamic_cast<DockSplitNode*> (n))
            {
                for (int i = sp->getNumChildren() - 1; i >= 0; --i)
                {
                    auto* c = sp->getChild (i);
                    if (auto* tab = dynamic_cast<DockTabNode*> (c))
                    {
                        if (tab->isEmpty())
                        {
                            sp->removeChild (i);
                            changed = true;
                        }
                    }
                    else
                    {
                        dropEmptyTabs (c);
                    }
                }
            }
        };
        dropEmptyTabs (rootNode.get());

        // 2. Collapse single-child non-root split nodes.
        DockSplitNode* collapseCandidate = nullptr;
        std::function<void (DockSplitNode*)> findCollapse;
        findCollapse = [&] (DockSplitNode* sp)
        {
            if (collapseCandidate != nullptr) return;
            if (sp != rootNode.get() && sp->getNumChildren() == 1)
            {
                collapseCandidate = sp;
                return;
            }
            for (int i = 0; i < sp->getNumChildren(); ++i)
                if (auto* child = dynamic_cast<DockSplitNode*> (sp->getChild (i)))
                    findCollapse (child);
        };
        findCollapse (rootNode.get());
        if (collapseCandidate != nullptr)
        {
            if (auto* parent = dynamic_cast<DockSplitNode*> (collapseCandidate->getParentComponent()))
            {
                const int idx     = parent->indexOf (collapseCandidate);
                const int sz      = parent->getChildSize (idx);
                auto lone         = collapseCandidate->removeChild (0);
                parent->removeChild (idx);
                if (lone) parent->addChild (std::move (lone), sz, idx);
                changed = true;
            }
        }

        // 3. Drop dead floating windows (no tab node, or empty tab node).
        for (auto it = floatingWindows.begin(); it != floatingWindows.end(); )
        {
            auto* tn = (*it)->getTabNode();
            if (tn == nullptr || tn->isEmpty())
            {
                it = floatingWindows.erase (it);
                changed = true;
            }
            else
            {
                ++it;
            }
        }
    }

    if (rootNode) rootNode->resized();
}

//----- Drag flow ------------------------------------------------------------

inline DockTabNode* DockManager::findTabNodeAtScreen (juce::Point<int> screenPos)
{
    if (! getScreenBounds().contains (screenPos)) return nullptr;
    std::vector<DockTabNode*> nodes;
    rootNode->collectTabNodes (nodes);
    for (auto* n : nodes)
        if (n->getScreenBounds().contains (screenPos))
            return n;
    return nullptr;
}

inline juce::Rectangle<int> DockManager::previewRectFor (DockSide side, bool outer, DockTabNode* hovered)
{
    if (side == DockSide::None || overlay == nullptr) return {};

    if (outer)
    {
        auto r = overlay->getLocalBounds();
        switch (side)
        {
            case DockSide::Left:   return r.removeFromLeft   (r.getWidth()  / 2);
            case DockSide::Right:  return r.removeFromRight  (r.getWidth()  / 2);
            case DockSide::Top:    return r.removeFromTop    (r.getHeight() / 2);
            case DockSide::Bottom: return r.removeFromBottom (r.getHeight() / 2);
            default: return {};
        }
    }

    if (hovered == nullptr) return {};

    const auto sb = hovered->getScreenBounds();
    const auto tl = overlay->getLocalPoint (nullptr, sb.getTopLeft());
    auto r = juce::Rectangle<int> (tl.x, tl.y, sb.getWidth(), sb.getHeight());

    switch (side)
    {
        case DockSide::Center: return r;
        case DockSide::Left:   return r.removeFromLeft   (r.getWidth()  / 2);
        case DockSide::Right:  return r.removeFromRight  (r.getWidth()  / 2);
        case DockSide::Top:    return r.removeFromTop    (r.getHeight() / 2);
        case DockSide::Bottom: return r.removeFromBottom (r.getHeight() / 2);
        default: return {};
    }
}

inline void DockManager::updateDragOverlay (juce::Point<int> screenPos)
{
    if (overlay == nullptr) return;
    overlay->clear();
    overlay->addOuterGuides (overlay->getLocalBounds());

    hoveredTabNode = findTabNodeAtScreen (screenPos);
    if (hoveredTabNode != nullptr)
    {
        const auto centreScreen = hoveredTabNode->getScreenBounds().getCentre();
        const auto centreLocal  = overlay->getLocalPoint (nullptr, centreScreen);
        overlay->addInnerGuides (centreLocal);
    }

    const auto cursorLocal = overlay->getLocalPoint (nullptr, screenPos);
    const auto hit         = overlay->hitTest (cursorLocal);
    overlay->setHoverSide (hit.first, hit.second);
    overlay->setPreview   (previewRectFor (hit.first, hit.second, hoveredTabNode));
}

inline void DockManager::beginTabDrag (DockablePanel* p, const juce::MouseEvent& e)
{
    if (dragMode != DragMode::None || p == nullptr) return;
    dragMode     = DragMode::GhostFromTab;
    draggedPanel = p;

    ghost = std::make_unique<DockDragGhost> (p->getPanelName(), theme);
    ghost->setScreenPos (e.getScreenPosition());
    ghost->setVisible (true);

    if (overlay) overlay->showOver (getScreenBounds());
    updateDragOverlay (e.getScreenPosition());

    juce::Desktop::getInstance().addGlobalMouseListener (this);
}

inline void DockManager::beginFloatingDrag (FloatingDockWindow* fw, const juce::MouseEvent& e)
{
    if (dragMode != DragMode::None || fw == nullptr) return;
    dragMode            = DragMode::FloatingWindow;
    draggedFloating     = fw;
    floatingDragOffset  = e.getScreenPosition() - fw->getScreenPosition();

    if (overlay) overlay->showOver (getScreenBounds());
    updateDragOverlay (e.getScreenPosition());

    juce::Desktop::getInstance().addGlobalMouseListener (this);
}

inline void DockManager::mouseDrag (const juce::MouseEvent& e)
{
    if (dragMode == DragMode::None) return;
    const auto screenPos = e.getScreenPosition();

    if (dragMode == DragMode::GhostFromTab)
    {
        if (ghost) ghost->setScreenPos (screenPos);
    }
    else if (dragMode == DragMode::FloatingWindow)
    {
        if (draggedFloating)
            draggedFloating->setTopLeftPosition (screenPos - floatingDragOffset);
    }
    updateDragOverlay (screenPos);
}

inline void DockManager::mouseUp (const juce::MouseEvent& e)
{
    if (dragMode == DragMode::None) return;
    commitDrop (e.getScreenPosition());
    endDrag();
}

inline void DockManager::commitDrop (juce::Point<int> screenPos)
{
    if (overlay == nullptr) return;
    const auto cursorLocal = overlay->getLocalPoint (nullptr, screenPos);
    const auto hit         = overlay->hitTest (cursorLocal);
    const DockSide side    = hit.first;
    const bool    outer    = hit.second;

    if (dragMode == DragMode::GhostFromTab)
    {
        if (draggedPanel == nullptr) return;

        if (side != DockSide::None)
        {
            if (outer)            dockPanelAtEdge (draggedPanel, side);
            else                  dockPanel       (draggedPanel, hoveredTabNode, side);
        }
        else if (! getScreenBounds().contains (screenPos))
        {
            floatPanel (draggedPanel,
                        screenPos - juce::Point<int> (60, 12),
                        { 360, 240 });
        }
    }
    else if (dragMode == DragMode::FloatingWindow)
    {
        if (draggedFloating == nullptr) return;

        if (side != DockSide::None)
        {
            auto sourceTab = draggedFloating->releaseTabNode();
            if (sourceTab)
            {
                if (outer) dockTabsAtEdge   (std::move (sourceTab), side);
                else       dockTabsRelative (std::move (sourceTab), hoveredTabNode, side);
            }
            for (auto it = floatingWindows.begin(); it != floatingWindows.end(); ++it)
            {
                if (it->get() == draggedFloating)
                {
                    floatingWindows.erase (it);
                    break;
                }
            }
        }
    }
}

inline void DockManager::endDrag()
{
    juce::Desktop::getInstance().removeGlobalMouseListener (this);
    if (overlay)
    {
        overlay->clear();
        overlay->hide();
    }
    ghost.reset();
    dragMode        = DragMode::None;
    draggedPanel    = nullptr;
    draggedFloating = nullptr;
    hoveredTabNode  = nullptr;
}

//----- Save / Load ----------------------------------------------------------

inline juce::ValueTree DockManager::saveLayout() const
{
    juce::ValueTree v (ids::Layout);
    if (rootNode) v.appendChild (rootNode->saveState(), nullptr);

    juce::ValueTree fl (ids::Floating);
    for (const auto& fw : floatingWindows)
        fl.appendChild (saveFloatingWindow (fw.get()), nullptr);
    v.appendChild (fl, nullptr);
    return v;
}

inline juce::ValueTree DockManager::saveFloatingWindow (FloatingDockWindow* fw) const
{
    juce::ValueTree v (ids::Window);
    const auto sb = fw->getScreenBounds();
    v.setProperty (ids::X, sb.getX(),      nullptr);
    v.setProperty (ids::Y, sb.getY(),      nullptr);
    v.setProperty (ids::W, sb.getWidth(),  nullptr);
    v.setProperty (ids::H, sb.getHeight(), nullptr);
    if (auto* tn = fw->getTabNode())
        v.appendChild (tn->saveState(), nullptr);
    return v;
}

inline std::unique_ptr<DockNode> DockManager::loadNode (const juce::ValueTree& v)
{
    if (v.hasType (ids::Tabs))
    {
        auto tn = std::make_unique<DockTabNode> (*this);
        const int active = (int) v.getProperty (ids::ActiveIndex, -1);
        for (int i = 0; i < v.getNumChildren(); ++i)
        {
            auto pv = v.getChild (i);
            if (pv.hasType (ids::Panel))
            {
                const auto pid = pv.getProperty (ids::PanelId).toString();
                if (auto* p = findPanel (pid))
                    tn->addPanel (p);
            }
        }
        tn->setActiveIndex (juce::jlimit (-1, tn->getNumPanels() - 1, active));
        return tn;
    }

    if (v.hasType (ids::Split))
    {
        const auto o = orientationFromString (v.getProperty (ids::Orientation).toString());
        auto sn = std::make_unique<DockSplitNode> (*this, o);

        juce::StringArray szTokens;
        szTokens.addTokens (v.getProperty (ids::Sizes).toString(), ",", "");
        for (int i = 0; i < v.getNumChildren(); ++i)
        {
            auto child = loadNode (v.getChild (i));
            const int sz = (i < szTokens.size()) ? szTokens[i].getIntValue() : 100;
            if (child) sn->addChild (std::move (child), sz);
        }
        return sn;
    }
    return nullptr;
}

inline void DockManager::loadLayout (const juce::ValueTree& state)
{
    if (! state.hasType (ids::Layout)) return;

    // Detach every panel from the current layout (without deleting).
    for (auto* p : allPanels)
        detachPanelFromLayout (p);

    floatingWindows.clear();

    // Rebuild root.
    if (rootNode) removeChildComponent (rootNode.get());
    rootNode = std::make_unique<DockSplitNode> (*this, SplitOrientation::Horizontal);
    addAndMakeVisible (*rootNode);

    if (state.getNumChildren() > 0)
    {
        const auto rootStored = state.getChild (0);
        if (rootStored.hasType (ids::Split))
        {
            const auto o = orientationFromString (rootStored.getProperty (ids::Orientation).toString());
            rootNode->setOrientation (o);

            juce::StringArray szTokens;
            szTokens.addTokens (rootStored.getProperty (ids::Sizes).toString(), ",", "");
            for (int i = 0; i < rootStored.getNumChildren(); ++i)
            {
                auto child = loadNode (rootStored.getChild (i));
                const int sz = (i < szTokens.size()) ? szTokens[i].getIntValue() : 100;
                if (child) rootNode->addChild (std::move (child), sz);
            }
        }
        else if (rootStored.hasType (ids::Tabs))
        {
            auto tn = loadNode (rootStored);
            if (tn) rootNode->addChild (std::move (tn), 200);
        }
    }

    // Floating windows.
    for (int c = 0; c < state.getNumChildren(); ++c)
    {
        const auto fl = state.getChild (c);
        if (! fl.hasType (ids::Floating)) continue;
        for (int i = 0; i < fl.getNumChildren(); ++i)
        {
            const auto wv = fl.getChild (i);
            if (! wv.hasType (ids::Window) || wv.getNumChildren() == 0) continue;

            auto node = loadNode (wv.getChild (0));
            std::unique_ptr<DockTabNode> tn (dynamic_cast<DockTabNode*> (node.release()));
            if (tn == nullptr || tn->isEmpty()) continue;

            auto fw = std::make_unique<FloatingDockWindow> (*this);
            fw->setTabNode (std::move (tn));
            fw->setBounds ((int) wv.getProperty (ids::X, 100),
                           (int) wv.getProperty (ids::Y, 100),
                           (int) wv.getProperty (ids::W, 360),
                           (int) wv.getProperty (ids::H, 240));
            floatingWindows.push_back (std::move (fw));
        }
    }

    // Place any unreferenced panels in the first tab node.
    for (auto* p : allPanels)
    {
        bool placed = false;
        std::vector<DockTabNode*> nodes;
        rootNode->collectTabNodes (nodes);
        for (auto* n : nodes) if (n->indexOf (p) >= 0) { placed = true; break; }
        if (! placed)
            for (auto& fw : floatingWindows)
                if (auto* tn = fw->getTabNode())
                    if (tn->indexOf (p) >= 0) { placed = true; break; }
        if (! placed)
            firstTabNodeOrCreate()->addPanel (p);
    }

    simplifyTree();
    resized();
}

} // namespace docking
