/*
  ==============================================================================

    MoveRegionAction.h
    Created: 17 Oct 2025 6:49:40pm
    Author:  mpue

  ==============================================================================
*/

#pragma once

/*
  ==============================================================================

    MoveRegionAction.h
    Created: 17 Oct 2025
    Author:  mpue

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class Region;

class MoveRegionAction : public UndoableAction
{
public:
    MoveRegionAction(Region* region, long oldSampleOffset, long newSampleOffset);
    ~MoveRegionAction();

    bool perform() override;
    bool undo() override;

private:
    Region* region;
    long oldSampleOffset;
    long newSampleOffset;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MoveRegionAction)
};