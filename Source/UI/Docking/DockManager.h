//============================================================================
//  DockManager.h
//  The top-level coordinator.  Owns every panel, the main dock-area root
//  node and any number of floating windows.  Drives the drag overlay,
//  persists layout to/from juce::ValueTree.
//
//  Method bodies are all in DockingFramework.h - this header is just the
//  shape of the API.  Include DockingFramework.h to actually use it.
//============================================================================
#pragma once

#include "DockTypes.h"
#include "DockablePanel.h"
#include "DockNode.h"
#include "DockOverlay.h"
#include "FloatingDockWindow.h"

namespace docking
{

//----------------------------------------------------------------------------
//  Small helper component used as the visual cue while dragging a tab.
//----------------------------------------------------------------------------
class DockDragGhost : public juce::Component
{
public:
    DockDragGhost (const juce::String& titleText, const DockTheme& t)
        : theme (t), title (titleText)
    {
        setOpaque (false);
        setInterceptsMouseClicks (false, false);
        setAlwaysOnTop (true);
        setSize (180, 24);
        addToDesktop (juce::ComponentPeer::windowIsTemporary
                    | juce::ComponentPeer::windowIgnoresMouseClicks);
    }

    void paint (juce::Graphics& g) override
    {
        g.setOpacity (0.85f);
        g.fillAll (theme.headerActive);
        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.drawRect (getLocalBounds(), 1);
        g.setOpacity (1.0f);
        g.setColour (theme.headerText);
        g.setFont (juce::Font (juce::FontOptions ((float) getHeight() * 0.55f)));
        g.drawText (title, getLocalBounds().reduced (8, 0),
                    juce::Justification::centredLeft, true);
    }

    void setScreenPos (juce::Point<int> p)
    {
        setTopLeftPosition (p.x + 10, p.y + 14);
    }

private:
    const DockTheme& theme;
    juce::String title;
};

//============================================================================
//  DockManager
//============================================================================
class DockManager : public juce::Component
{
public:
    DockManager();
    ~DockManager() override;

    //----- Theme ----------------------------------------------------------
    const DockTheme& getTheme()        const noexcept { return theme; }
    DockTheme&       getMutableTheme() noexcept       { return theme; }
    void repaintAll();

    //----- Panel API ------------------------------------------------------

    /** Creates and registers a new panel.  Manager retains ownership.
        `initialSide` controls where it is placed if a layout already exists:
        Center => tabify into any existing tab node, otherwise a brand-new tab
        Left/Right/Top/Bottom => dock at the outer edge of the main area. */
    DockablePanel* createPanel (const juce::String& id,
                                const juce::String& displayName,
                                std::unique_ptr<juce::Component> content,
                                DockSide initialSide = DockSide::Center);

    /** Look up a registered panel by ID. */
    DockablePanel* findPanel (const juce::String& id) const;

    /** Removes a panel from the layout and deletes it. */
    void closePanel (DockablePanel* p);
    void closePanelById (const juce::String& id) { closePanel (findPanel (id)); }

    /** Brings a panel to the front, making it the active tab in its node
        (or focusing its floating window). */
    void activatePanel (DockablePanel* p);

    /** Number / list of registered panels. */
    int  getNumPanels() const noexcept { return allPanels.size(); }
    DockablePanel* getPanel (int i) const { return allPanels[i]; }

    //----- Layout API -----------------------------------------------------

    /** Docks a panel relative to a target tab node.  `side == Center` tabifies. */
    void dockPanel (DockablePanel* p, DockTabNode* target, DockSide side);

    /** Docks a panel at the outer edge of the main dock area. */
    void dockPanelAtEdge (DockablePanel* p, DockSide side);

    /** Pops a panel out into a new floating window at the given screen
        position (top-left). */
    FloatingDockWindow* floatPanel (DockablePanel* p, juce::Point<int> screenPos,
                                    juce::Point<int> size = { 360, 240 });

    //----- Maximize -------------------------------------------------------

    /** Toggles maximize for a docked tab node: pulls it out of the layout
        tree so it fills the whole dock area, or restores it if it (or any
        other node) is already maximized. No-op for nodes that don't live in
        the main dock tree (e.g. inside a floating window). */
    void toggleMaximizeTabNode (DockTabNode* node);

    /** Restores a maximized node back into its original tree slot. No-op if
        nothing is maximized. Called automatically before structural changes
        so the layout tree is never persisted/mutated in a maximized state. */
    void restoreMaximizedNode();

    bool hasMaximizedNode() const noexcept { return maximizedNode != nullptr; }

    //----- Persistence ----------------------------------------------------

    /** Captures the current layout as a juce::ValueTree.  Panels are
        referenced by their ID; their content components are not stored. */
    juce::ValueTree saveLayout() const;

    /** Restores a layout that was previously produced by saveLayout().
        Panels that aren't currently registered are silently skipped;
        panels that exist but aren't mentioned by the layout are appended
        to the first tab node (or a new one if the area is empty). */
    void loadLayout (const juce::ValueTree& state);

    //----- Internal - called by DockTabNode / FloatingDockWindow ---------
    void beginTabDrag      (DockablePanel* p, const juce::MouseEvent& e);
    void beginFloatingDrag (FloatingDockWindow* fw, const juce::MouseEvent& e);
    void requestClosePanel (DockablePanel* p)         { closePanel (p); }
    void closeFloatingWindow (FloatingDockWindow* fw);

    //----- juce::Component -----------------------------------------------
    void paint   (juce::Graphics& g) override;
    void resized() override;

    //----- juce::MouseListener -------------------------------------------
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp   (const juce::MouseEvent& e) override;

    //----- Internal helpers exposed for tests / introspection -------------
    DockSplitNode* getRootNode() const noexcept { return rootNode.get(); }
    DockOverlay*   getOverlay()  const noexcept { return overlay.get(); }
    const std::vector<std::unique_ptr<FloatingDockWindow>>& getFloatingWindows() const noexcept
    {
        return floatingWindows;
    }

private:
    //----- Members --------------------------------------------------------
    DockTheme                                              theme;
    juce::OwnedArray<DockablePanel>                        allPanels;
    std::unique_ptr<DockSplitNode>                         rootNode;
    std::vector<std::unique_ptr<FloatingDockWindow>>       floatingWindows;
    std::unique_ptr<DockOverlay>                           overlay;

    enum class DragMode { None, GhostFromTab, FloatingWindow };
    DragMode             dragMode = DragMode::None;
    DockablePanel*       draggedPanel = nullptr;
    FloatingDockWindow*  draggedFloating = nullptr;
    juce::Point<int>     floatingDragOffset;          // mouseDown - window topLeft
    std::unique_ptr<DockDragGhost> ghost;
    DockTabNode*         hoveredTabNode = nullptr;

    //----- Maximize state -------------------------------------------------
    DockTabNode*              maximizedNode       = nullptr; // == maximizedHeld.get()
    std::unique_ptr<DockNode> maximizedHeld;                 // owns node while pulled out
    DockSplitNode*            maximizedOrigParent = nullptr;
    int                       maximizedOrigIndex  = 0;
    int                       maximizedOrigSize   = 0;

    //----- Helpers --------------------------------------------------------
    void                 addPanelInitially (DockablePanel* p, DockSide side);
    DockTabNode*         firstTabNodeOrCreate();
    DockTabNode*         findTabNodeAtScreen (juce::Point<int> screenPos);
    juce::Rectangle<int> previewRectFor (DockSide side, bool outer, DockTabNode* hovered);
    void                 updateDragOverlay (juce::Point<int> screenPos);
    void                 commitDrop (juce::Point<int> screenPos);
    void                 endDrag();
    void                 simplifyTree();
    void                 detachPanelFromLayout (DockablePanel* p);
    void                 dockTabsRelative (std::unique_ptr<DockTabNode> source,
                                           DockTabNode* target, DockSide side);
    void                 dockTabsAtEdge   (std::unique_ptr<DockTabNode> source, DockSide side);
    void                 ensureRootOrientation (SplitOrientation o);
    juce::ValueTree      saveFloatingWindow (FloatingDockWindow* fw) const;
    std::unique_ptr<DockNode> loadNode (const juce::ValueTree& v);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DockManager)
};

} // namespace docking
