/*
  ==============================================================================

    ResizeRegionAction.h
    Created: 16 May 2026
    Author:  mpue

    Undoable resize gesture (left- or right-edge) for any Region subclass.
    Captures and restores the triple (sampleOffset, sourceStartSample,
    numSamples) — right-edge resizes only touch numSamples, left-edge
    resizes touch all three. The subclass applies the state via
    Region::applyResizeState, which also re-derives bounds.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../Tracks/Region.h"

class ResizeRegionAction : public juce::UndoableAction
{
public:
    ResizeRegionAction(Region* region,
                       Region::ResizeState before,
                       Region::ResizeState after);
    ~ResizeRegionAction() override = default;

    bool perform() override;
    bool undo() override;

private:
    Region* region;
    Region::ResizeState before;
    Region::ResizeState after;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ResizeRegionAction)
};
