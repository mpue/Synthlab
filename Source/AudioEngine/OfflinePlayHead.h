/*
  ==============================================================================

    OfflinePlayHead.h
    Created: 15 Jul 2025 7:43:06pm
    Author:  mpue

  ==============================================================================
*/
#pragma once
#include "../JuceLibraryCode/JuceHeader.h"

struct OfflinePlayHead : public juce::AudioPlayHead
{
    juce::AudioPlayHead::PositionInfo info;

    virtual Optional<PositionInfo> getPosition() const
    {
        return info;
    }

    void setPositionInfo(int64_t samplePos, double sampleRate, double bpm)
    {
        double timeInSeconds = samplePos / sampleRate;
        double ppq = timeInSeconds * (bpm / 60.0);

        info.setTimeInSamples(samplePos);
        info.setTimeInSeconds(samplePos / sampleRate);
        info.setBpm(bpm);
		info.setTimeSignature(TimeSignature(4, 4)); // Default time signature

        info.setPpqPosition(ppq);
        info.setPpqPositionOfLastBarStart(0.0);

        info.setIsPlaying(true);
        info.setIsRecording(false);
    }

    void setRecording(bool recording)
    {
        info.setIsRecording(recording);
	}

    void setPlaying(bool playing)
    {
        info.setIsPlaying(playing);
    }

};
