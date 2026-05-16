/*
  ==============================================================================

    jcf_multi_selection.h
    Created: 25 Feb 2016 9:00:28am
    Author:  Jim Credland

  ==============================================================================
*/

#ifndef JCF_MULTI_SELECTION_H_INCLUDED
#define JCF_MULTI_SELECTION_H_INCLUDED


#include "../JuceLibraryCode/JuceHeader.h"
#include "UITools.h"

/**
 * MultiComponentDragger allows the user to select objects and drag them around the
 * screen.  Multiple objects can be selected and dragged at once.  The behaviour
 * is similar to Microsoft PowerPoint and probably lots of other applications.
 *
 * Holding down Command(Control) or Shift allows multiple selection.  Holding down
 * shift can optionally also constrain the objects movement to only the left or
 * right axis.
 *
 * The movement can be constrained to be within the bounds of the parent component.
 *
 * Objects directly attached to the desktop are not supported.
 *
 * Using: see handleMouseUp, handleMouseDown and handleMouseDrag
 *
 * You will probably also want to check isSelected() in your objects paint(Graphics &)
 * routine and ensure selected objects are highlighted.  Repaints are triggered
 * automatically if the selection status changes.
 *
 * @TODO: Add 'grid' support.
 */
class MultiComponentDragger
{
public:
    virtual ~MultiComponentDragger() {}

    void setConstrainBoundsToParent(bool shouldConstrainToParentSize,
                                    juce::BorderSize<int> amountPermittedOffscreen_)
    {
        constrainToParent = shouldConstrainToParentSize;
        amountPermittedOffscreen = amountPermittedOffscreen_;
    }

    /**
     If this flag is set then the dragging behaviour when shift
     is held down will be constrained to the vertical or horizontal
     direction.  This the the behaviour of Microsoft PowerPoint.
     */
    void setShiftConstrainsDirection(bool constrainDirection)
    {
        shiftConstrainsDirection = constrainDirection;
    }

    /**
     * Adds a specified component as being selected.
     */
    void setSelected(juce::Component * component, bool shouldNowBeSelected)
    {
        /* Asserts here? This class is only designed to work for components that have a common parent. */
        // jassert(selectedComponents.size() == 0 || component->getParentComponent() == selectedComponents[0]->getParentComponent());

        bool isAlreadySelected = isSelected(component);
        
        if (! isAlreadySelected && shouldNowBeSelected)
            selectedComponents.push_back(component);
        
        if (isAlreadySelected && ! shouldNowBeSelected)
            removeSelectedComponent(component);
    }

    /** Toggles the selected status of a particular component. */
    void toggleSelection(juce::Component * component)
    {
        setSelected(component, ! isSelected(component));
    }
    
    /** 
     You should call this when the user clicks on the background of the
     parent component.
     */
    void deselectAll()
    {
        for (auto c: selectedComponents)
            if (c)
                c->repaint();
        
        selectedComponents.clear();
    }
    
    /**
     Find out if a component is marked as selected.
     */
    bool isSelected(juce::Component * component) const
    {
        return std::find(selectedComponents.begin(),
                         selectedComponents.end(),
                         component) != selectedComponents.end();
    }

    /**
     Currently dragged "leader" component (the one the user grabbed). Other
     selected components are translated alongside it. Returns nullptr when no
     drag is active.
     */
    juce::Component* getComponentBeingDragged() const { return componentBeingDragged; }

    /**
     Read-only access to the current selection. Callers should treat weak
     refs as transient (components may be deleted underneath the dragger).
     Used e.g. by Region to push one undo action per moved region in a
     multi-region drag.
     */
    const std::vector<juce::WeakReference<juce::Component>>& getSelectedComponents() const
    {
        return selectedComponents;
    }

    /**
     True while more than one selected component is being dragged at once.
     Subclasses with per-component snap/clamp logic in componentMovedOrResized
     should consult this and skip per-component adjustments for non-leader
     components — otherwise each follower snaps to its own grid position and
     the group spreads out instead of moving rigidly.
     */
    bool isMultiDragInProgress() const
    {
        return didStartDragging && selectedComponents.size() > 1 && componentBeingDragged != nullptr;
    }
    
    /** 
     Call this from your components mouseDown event.
     */
    void handleMouseDown (juce::Component* component, const juce::MouseEvent & e)
    {
        jassert (component != nullptr);

        if (! isSelected(component))
        {
            if (! (e.mods.isShiftDown() || e.mods.isCommandDown()))
                deselectAll();
            
            setSelected(component, true);
            didJustSelect = true;
        }
        
        if (component != nullptr)
            mouseDownWithinTarget = e.getEventRelativeTo (component).getMouseDownPosition();

        componentBeingDragged = component;

        totalDragDelta = {0, 0};

        constrainedDirection = xAxisOnly;
        
        component->repaint();
    }
    
    /**
     Call this from your components mouseUp event.
     */
    void handleMouseUp (juce::Component* component, const juce::MouseEvent & e)
    {
        if (didStartDragging)
            didStartDragging = false;
        else
            if (!didJustSelect && isSelected(component))
                setSelected(component, false);
        
        didJustSelect = false;
        
        component->repaint();
    }

    /**
     Call this from your components mouseDrag event.
     */
    void handleMouseDrag (const juce::MouseEvent& e)
    {

        jassert (e.mods.isAnyMouseButtonDown()); // The event has to be a drag event!

        /** Ensure tiny movements don't start a drag. */
        if (!didStartDragging && e.getDistanceFromDragStart() < minimumMovementToStartDrag)
            return;

        didStartDragging = true;

        juce::Point<int> delta = e.getEventRelativeTo (componentBeingDragged).getPosition() - mouseDownWithinTarget;
        
        if (constrainToParent)
        {
            auto targetArea = getAreaOfSelectedComponents() + delta;
            auto limit = componentBeingDragged->getParentComponent()->getBounds();

            amountPermittedOffscreen.subtractFrom(targetArea);

            if (targetArea.getX() < 0)
                delta.x -= targetArea.getX();

            if (targetArea.getY() < 0)
                delta.y -= targetArea.getY();

            if (targetArea.getBottom() > limit.getBottom())
                delta.y -= targetArea.getBottom() - limit.getBottom();

            if (targetArea.getRight() > limit.getRight())
                delta.x -= targetArea.getRight() - limit.getRight();
        }

        
        applyDirectionConstraints(e, delta);
        
        
        if (e.mods.isAltDown()) {
            return;
        }
        
        if (!e.mods.isShiftDown())
            delta.x = (int)UITools::snap(delta.x, raster, tolerance);

        if (lockHorizontal)
            delta.y = -totalDragDelta.y;

        // Bei mehreren selektierten Komponenten zuerst den Leader bewegen,
        // dann seinen tatsaechlich realisierten Delta (kann durch Per-
        // Component-Snap/Clamp im componentMovedOrResized abweichen) auf alle
        // Follower anwenden. Sonst raster jeder Follower individuell auf sein
        // eigenes Grid-Ziel und die Gruppe driftet auseinander.
        if (selectedComponents.size() > 1 && componentBeingDragged != nullptr)
        {
            const juce::Point<int> leaderBefore = componentBeingDragged->getBounds().getPosition();
            componentBeingDragged->setBounds(componentBeingDragged->getBounds() + delta);
            const juce::Point<int> leaderAfter = componentBeingDragged->getBounds().getPosition();
            const juce::Point<int> effective = leaderAfter - leaderBefore;

            for (auto comp : selectedComponents)
            {
                if (comp != nullptr && comp != componentBeingDragged)
                    comp->setBounds(comp->getBounds() + effective);
            }

            totalDragDelta += effective;
        }
        else
        {
            for (auto comp: selectedComponents)
            {
                if (comp != nullptr)
                {
                    juce::Rectangle<int> bounds (comp->getBounds());

                    bounds += delta;

                    comp->setBounds (bounds);
                }
            }

            totalDragDelta += delta;
        }
    }
    
    void setLockHorizontal(bool lock) { lockHorizontal = lock; }

    void setRaster(int raster) {
        this->raster = raster;
    }
    
    void setTolerance(int tolerance) {
        this->tolerance = tolerance;
    }
    
    void setMaxX(int maxX) {
        this->maxX = maxX;
    }
    
private:
    
    juce::Rectangle<int> getAreaOfSelectedComponents()
    {
        if (selectedComponents.size() == 0)
            return juce::Rectangle<int>(0, 0, 0, 0);
        
        juce::Rectangle<int> a = selectedComponents[0]->getBounds();
        
        for (auto comp: selectedComponents)
            if (comp)
                a = a.getUnion(comp->getBounds());
        
        return a;
    }


    void applyDirectionConstraints(const juce::MouseEvent &e, juce::Point<int> &delta)
    {
        if (shiftConstrainsDirection && e.mods.isShiftDown())
        {
            /* xy > 0 == movement mainly X direction, xy < 0 == movement mainly Y direction. */
            int xy = abs(totalDragDelta.x + delta.x) - abs(totalDragDelta.y + delta.y);

            /* big movements remove the lock to a particular axis */

            if (xy > minimumMovementToStartDrag)
                constrainedDirection = xAxisOnly;

            if (xy < -minimumMovementToStartDrag)
                constrainedDirection = yAxisOnly;

            if ((xy > 0 && constrainedDirection != yAxisOnly)
                ||
                (constrainedDirection == xAxisOnly))
            {
                delta.y = -totalDragDelta.y; /* move X direction only. */
                constrainedDirection = xAxisOnly;
            }
            else if ((xy <= 0 && constrainedDirection != xAxisOnly)
                     ||
                     constrainedDirection == yAxisOnly)
            {
                delta.x = -totalDragDelta.x; /* move Y direction only. */
                constrainedDirection = yAxisOnly;
            }
            else
            {
                delta = {0, 0};
            }
        }
        else
        {
            constrainedDirection = xAxisOnly;

        }
    }

    void removeSelectedComponent(juce::Component * component)
    {
        selectedComponents.erase(std::remove(selectedComponents.begin(),
                                             selectedComponents.end(),
                                             component), selectedComponents.end());
    }
    
    enum
    {
        noConstraint,
        xAxisOnly,
        yAxisOnly
    } constrainedDirection;

    const int minimumMovementToStartDrag = 10;

    bool constrainToParent {true};
    bool shiftConstrainsDirection {false};
    bool lockHorizontal {false};

    bool didJustSelect {false};
    bool didStartDragging {false};

    juce::Point<int> mouseDownWithinTarget;
    juce::Point<int> totalDragDelta;

    std::vector<juce::WeakReference<juce::Component>> selectedComponents;
    juce::Component * componentBeingDragged { nullptr };
    
    juce::BorderSize<int> amountPermittedOffscreen;
   
    int raster = 1;
    int tolerance = 4;
    int maxX = 0;
};



#endif  // JCF_MULTI_SELECTION_H_INCLUDED
