/*
  ==============================================================================

    SplitRegionAction.h
    Created: 24 Apr 2026
    Author:  mpue

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class Track;
class Region;

class SplitRegionAction : public juce::UndoableAction
{
public:
    SplitRegionAction(Track* track, Region* original, Region* left, Region* right);
    ~SplitRegionAction();

    bool perform() override;
    bool undo() override;

private:
    Track*  track;
    Region* original;
    Region* left;
    Region* right;
    bool originalInTrack = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SplitRegionAction)
};
