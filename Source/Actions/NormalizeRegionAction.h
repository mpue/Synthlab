/*
  ==============================================================================

    NormalizeRegionAction.h
    Created: 17 Oct 2025 5:57:42pm
    Author:  mpue

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class TrackNavigator;
class AudioRegion;

class NormalizeRegionAction : public UndoableAction
{
public:
    NormalizeRegionAction(TrackNavigator* navigator);
    ~NormalizeRegionAction();
    
    bool perform() override;
    bool undo() override;

private:
    TrackNavigator* navigator;
    AudioRegion* targetRegion = nullptr;
    std::unique_ptr<juce::AudioSampleBuffer> originalBuffer;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NormalizeRegionAction)
};
