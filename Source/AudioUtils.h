/*
  ==============================================================================

    AudioUtils.h
    Created: 15 Oct 2025 8:46:23pm
    Author:  mpue

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

namespace AudioUtils
{
    static void normalizeBuffer(AudioSampleBuffer& buffer)
    {
        // Finde den höchsten Absolutwert im gesamten Buffer
        float maxLevel = 0.0f;

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            auto* channelData = buffer.getReadPointer(channel);
            auto channelMax = FloatVectorOperations::findMaximum(channelData, buffer.getNumSamples());
            auto channelMin = FloatVectorOperations::findMinimum(channelData, buffer.getNumSamples());

            maxLevel = jmax(maxLevel, jmax(std::abs(channelMax), std::abs(channelMin)));
        }

        // Normalisiere nur wenn nötig (vermeide Division durch 0)
        if (maxLevel > 0.0f && maxLevel != 1.0f)
        {
            float gainFactor = 1.0f / maxLevel;
            buffer.applyGain(gainFactor);
        }
    }

    static void normalizeToLevel(AudioSampleBuffer& buffer, float targetLevel = 0.9f)
    {
        float maxLevel = 0.0f;

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            auto range = buffer.findMinMax(channel, 0, buffer.getNumSamples());
            maxLevel = jmax(maxLevel, jmax(std::abs(range.getStart()), std::abs(range.getEnd())));
        }

        if (maxLevel > 0.0f)
        {
            float gainFactor = targetLevel / maxLevel;
            buffer.applyGain(gainFactor);
        }
    }

    static void normalizeByRMS(AudioSampleBuffer& buffer, float targetRMS = 0.2f)
    {
        float totalRMS = 0.0f;

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            auto* channelData = buffer.getReadPointer(channel);
            float channelRMS = 0.0f;

            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            {
                channelRMS += channelData[sample] * channelData[sample];
            }

            channelRMS = std::sqrt(channelRMS / buffer.getNumSamples());
            totalRMS += channelRMS;
        }

        totalRMS /= buffer.getNumChannels(); // Durchschnitt aller Kanäle

        if (totalRMS > 0.0f)
        {
            float gainFactor = targetRMS / totalRMS;
            buffer.applyGain(gainFactor);
        }
    }

    static void normalizeChannelsIndividually(AudioSampleBuffer& buffer)
    {
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            auto range = buffer.findMinMax(channel, 0, buffer.getNumSamples());
            float maxLevel = jmax(std::abs(range.getStart()), std::abs(range.getEnd()));

            if (maxLevel > 0.0f && maxLevel != 1.0f)
            {
                float gainFactor = 1.0f / maxLevel;
                buffer.applyGain(channel, 0, buffer.getNumSamples(), gainFactor);
            }
        }
    }
}
