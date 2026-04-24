/*
  ==============================================================================

    MoveRegionAction.cpp
    Created: 17 Oct 2025 6:49:40pm
    Author:  mpue

  ==============================================================================
*/

#include "MoveRegionAction.h"

/*
  ==============================================================================

    MoveRegionAction.cpp
    Created: 17 Oct 2025
    Author:  mpue

  ==============================================================================
*/
#include "MoveRegionAction.h"
#include "../Tracks/Region.h"

MoveRegionAction::MoveRegionAction(Region* region, long oldSampleOffset, long newSampleOffset)
    : region(region), oldSampleOffset(oldSampleOffset), newSampleOffset(newSampleOffset)
{
}

MoveRegionAction::~MoveRegionAction()
{
}

bool MoveRegionAction::perform()
{
    if (region != nullptr) {
        region->setSampleOffset(newSampleOffset, true, true);
        region->setZoom(region->getZoom());
        return true;
    }
    return false;
}

bool MoveRegionAction::undo()
{
    if (region != nullptr) {
        region->setSampleOffset(oldSampleOffset, true, true);
        region->setZoom(region->getZoom());
        return true;
    }
    return false;
}