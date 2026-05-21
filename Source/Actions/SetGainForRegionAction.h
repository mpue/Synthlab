/*
  ==============================================================================

    SetGainForRegionAction.h
    Created: 17 Oct 2025 5:58:36pm
    Author:  mpue

    Undoable per-region gain edit. Stores old/new linear gain multipliers
    and applies them via Region::setGain — the AudioRegion override mutates
    the playback multiplier, MIDI/Instrument regions silently no-op.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class Region;

class SetGainForRegionAction : public juce::UndoableAction
{
public:
    SetGainForRegionAction(Region* region, float oldGain, float newGain);
    ~SetGainForRegionAction() override;

    bool perform() override;
    bool undo() override;

private:
    Region* region;
    float oldGain;
    float newGain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SetGainForRegionAction)
};
