/*
  ==============================================================================

    FadeRegionAction.h
    Created: 17 Oct 2025 5:58:07pm
    Author:  mpue

  ==============================================================================
*/

#pragma once

#pragma once

#include <JuceHeader.h>

class TrackNavigator;
class AudioRegion;

class FadeRegionAction : public UndoableAction
{
public:
    FadeRegionAction(TrackNavigator* navigator);
    ~FadeRegionAction();

    bool perform() override;
    bool undo() override;

private:
    TrackNavigator* navigator;
    AudioRegion* targetRegion = nullptr;
    std::unique_ptr<juce::AudioSampleBuffer> originalBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FadeRegionAction)
};
