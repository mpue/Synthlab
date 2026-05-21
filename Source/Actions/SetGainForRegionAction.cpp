/*
  ==============================================================================

    SetGainForRegionAction.cpp
    Created: 17 Oct 2025 5:58:36pm
    Author:  mpue

  ==============================================================================
*/

#include "SetGainForRegionAction.h"
#include "../Tracks/Region.h"

SetGainForRegionAction::SetGainForRegionAction(Region* region, float oldGain, float newGain)
    : region(region), oldGain(oldGain), newGain(newGain)
{
}

SetGainForRegionAction::~SetGainForRegionAction()
{
}

bool SetGainForRegionAction::perform()
{
    if (region == nullptr) return false;
    region->setGain(newGain);
    return true;
}

bool SetGainForRegionAction::undo()
{
    if (region == nullptr) return false;
    region->setGain(oldGain);
    return true;
}
