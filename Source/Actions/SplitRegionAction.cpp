/*
  ==============================================================================

    SplitRegionAction.cpp
    Created: 24 Apr 2026
    Author:  mpue

  ==============================================================================
*/

#include "SplitRegionAction.h"
#include "../Tracks/Track.h"
#include "../Tracks/Region.h"

SplitRegionAction::SplitRegionAction(Track* track, Region* original, Region* left, Region* right)
    : track(track), original(original), left(left), right(right)
{
}

SplitRegionAction::~SplitRegionAction()
{
    if (originalInTrack) {
        delete left;
        delete right;
    } else {
        delete original;
    }
}

bool SplitRegionAction::perform()
{
    track->detachRegion(original);
    track->attachRegion(left);
    track->attachRegion(right);
    track->clearSelection();
    track->getDragger()->setSelected(left, true);
    track->getDragger()->setSelected(right, true);
    originalInTrack = false;
    track->repaint();
    return true;
}

bool SplitRegionAction::undo()
{
    track->detachRegion(left);
    track->detachRegion(right);
    track->attachRegion(original);
    track->clearSelection();
    track->getDragger()->setSelected(original, true);
    originalInTrack = true;
    track->repaint();
    return true;
}
