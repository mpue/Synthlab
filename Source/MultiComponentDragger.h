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
                                    BorderSize<int> amountPermittedOffscreen_)
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
    void setSelected(Component * component, bool shouldNowBeSelected)
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
    void toggleSelection(Component * component)
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
    bool isSelected(Component * component) const
    {
        return std::find(selectedComponents.begin(),
                         selectedComponents.end(),
                         component) != selectedComponents.end();
    }
    
    /** 
     Call this from your components mouseDown event.
     */
    void handleMouseDown (Component* component, const MouseEvent & e)
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
    void handleMouseUp (Component* component, const MouseEvent & e)
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
    void handleMouseDrag (const MouseEvent& e)
    {

        jassert (e.mods.isAnyMouseButtonDown()); // The event has to be a drag event!

        /** Ensure tiny movements don't start a drag. */
        if (!didStartDragging && e.getDistanceFromDragStart() < minimumMovementToStartDrag)
            return;

        didStartDragging = true;

        Point<int> delta = e.getEventRelativeTo (componentBeingDragged).getPosition() - mouseDownWithinTarget;
        
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
        
        delta.x = snap(delta.x, raster);

        for (auto comp: selectedComponents)
        {
            if (comp != nullptr)
            {
                Rectangle<int> bounds (comp->getBounds());

                bounds += delta;

                comp->setBounds (bounds);
            }
        }

        
        totalDragDelta += delta;
    }

    double snap(double location, double raster) {
        
        int toleranceWindow = (raster / tolerance);
        
        if (location > 0) {
            
            
            if ((fmod(location,raster )) > toleranceWindow) {
                location = location + (raster - (fmod(location,raster)));
            }
            else {
                location = location - (fmod(location,raster));
            }
        }
        else {
            if ((fmod(location,raster)) < toleranceWindow) {
                location = location + (raster - (fmod(location,raster))) - raster;
            }
            else {
                location = location - (fmod(location, raster)) - raster;
            }
        }
        
        return location;
    }
    
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
    
    Rectangle<int> getAreaOfSelectedComponents()
    {
        if (selectedComponents.size() == 0)
            return Rectangle<int>(0, 0, 0, 0);
        
        Rectangle<int> a = selectedComponents[0]->getBounds();
        
        for (auto comp: selectedComponents)
            if (comp)
                a = a.getUnion(comp->getBounds());
        
        return a;
    }


    void applyDirectionConstraints(const MouseEvent &e, Point<int> &delta)
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
            constrainedDirection = noConstraint;

        }
    }

    void removeSelectedComponent(Component * component)
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

    bool didJustSelect {false};
    bool didStartDragging {false};

    Point<int> mouseDownWithinTarget;
    Point<int> totalDragDelta;

    std::vector<WeakReference<Component>> selectedComponents;
    Component * componentBeingDragged { nullptr };
    
    BorderSize<int> amountPermittedOffscreen;
   
    int raster = 1;
    int tolerance = 4;
    int maxX = 0;
};



#endif  // JCF_MULTI_SELECTION_H_INCLUDED
