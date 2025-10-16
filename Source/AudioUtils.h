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
    static void reverseAudioBuffer(juce::AudioSampleBuffer& buffer)
    {
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();

        for (int channel = 0; channel < numChannels; ++channel)
        {
            float* channelData = buffer.getWritePointer(channel);

            // Umkehrung durch Vertauschen der Samples von beiden Enden
            for (int i = 0; i < numSamples / 2; ++i)
            {
                std::swap(channelData[i], channelData[numSamples - 1 - i]);
            }
        }
    }

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

    /**
     * Apply a linear fade-in to the beginning of the audio buffer
     * 
     * @param buffer The audio buffer to apply the fade to
     * @param fadeInSamples The length of the fade in samples
     */
    static void applyFadeIn(AudioSampleBuffer& buffer, int fadeInSamples)
    {
        if (fadeInSamples <= 0 || fadeInSamples > buffer.getNumSamples())
            return;

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            float* channelData = buffer.getWritePointer(channel);
            
            for (int sample = 0; sample < fadeInSamples; ++sample)
            {
                float gain = static_cast<float>(sample) / static_cast<float>(fadeInSamples);
                channelData[sample] *= gain;
            }
        }
    }

    /**
     * Apply a linear fade-out to the end of the audio buffer
     * 
     * @param buffer The audio buffer to apply the fade to
     * @param fadeOutSamples The length of the fade in samples
     */
    static void applyFadeOut(AudioSampleBuffer& buffer, int fadeOutSamples)
    {
        if (fadeOutSamples <= 0 || fadeOutSamples > buffer.getNumSamples())
            return;

        const int numSamples = buffer.getNumSamples();
        const int fadeStartSample = numSamples - fadeOutSamples;

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            float* channelData = buffer.getWritePointer(channel);
            
            for (int sample = 0; sample < fadeOutSamples; ++sample)
            {
                float gain = 1.0f - (static_cast<float>(sample) / static_cast<float>(fadeOutSamples));
                channelData[fadeStartSample + sample] *= gain;
            }
        }
    }

    /**
     * Apply both fade-in and fade-out to the audio buffer
     * 
     * @param buffer The audio buffer to apply the fades to
     * @param fadeInSamples The length of the fade-in in samples
     * @param fadeOutSamples The length of the fade-out in samples
     */
    static void applyFadeInOut(AudioSampleBuffer& buffer, int fadeInSamples, int fadeOutSamples)
    {
        applyFadeIn(buffer, fadeInSamples);
        applyFadeOut(buffer, fadeOutSamples);
    }

    /**
     * Apply a smooth (exponential) fade-in to the beginning of the audio buffer
     * 
     * @param buffer The audio buffer to apply the fade to
     * @param fadeInSamples The length of the fade in samples
     */
    static void applyExponentialFadeIn(AudioSampleBuffer& buffer, int fadeInSamples)
    {
        if (fadeInSamples <= 0 || fadeInSamples > buffer.getNumSamples())
            return;

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            float* channelData = buffer.getWritePointer(channel);
            
            for (int sample = 0; sample < fadeInSamples; ++sample)
            {
                float position = static_cast<float>(sample) / static_cast<float>(fadeInSamples);
                float gain = position * position; // quadratic curve for smoother fade
                channelData[sample] *= gain;
            }
        }
    }

    /**
     * Apply a smooth (exponential) fade-out to the end of the audio buffer
     * 
     * @param buffer The audio buffer to apply the fade to
     * @param fadeOutSamples The length of the fade in samples
     */
    static void applyExponentialFadeOut(AudioSampleBuffer& buffer, int fadeOutSamples)
    {
        if (fadeOutSamples <= 0 || fadeOutSamples > buffer.getNumSamples())
            return;

        const int numSamples = buffer.getNumSamples();
        const int fadeStartSample = numSamples - fadeOutSamples;

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            float* channelData = buffer.getWritePointer(channel);
            
            for (int sample = 0; sample < fadeOutSamples; ++sample)
            {
                float position = 1.0f - (static_cast<float>(sample) / static_cast<float>(fadeOutSamples));
                float gain = position * position; // quadratic curve for smoother fade
                channelData[fadeStartSample + sample] *= gain;
            }
        }
    }
}
