/*
  ==============================================================================

    ResizeRegionAction.cpp
    Created: 16 May 2026
    Author:  mpue

  ==============================================================================
*/

#include "ResizeRegionAction.h"

ResizeRegionAction::ResizeRegionAction(Region* r,
                                       Region::ResizeState b,
                                       Region::ResizeState a)
    : region(r), before(b), after(a)
{
}

bool ResizeRegionAction::perform()
{
    if (region == nullptr) return false;
    region->applyResizeState(after);
    return true;
}

bool ResizeRegionAction::undo()
{
    if (region == nullptr) return false;
    region->applyResizeState(before);
    return true;
}
