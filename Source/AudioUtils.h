/*
  ==============================================================================

    AudioUtils.h
    Created: 15 Oct 2025 8:46:23pm
    Author:  mpue

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "stretch/signalsmith-stretch.h"
#include "Settings.h"

namespace AudioUtils
{
    /**
     * Generic offline time-stretch and/or pitch-shift via signalsmith-stretch.
     * Returns a freshly allocated buffer the caller owns. Use the thin wrappers
     * stretchBufferRubberband (time only) and pitchShiftBufferRubberband
     * (pitch only) instead of calling this directly.
     *
     * @param input        Source buffer (>= 1 channel).
     * @param timeRatio    1.0 = same length; >1 longer; <1 shorter.
     * @param pitchScale   1.0 = same pitch; 2^(semitones/12) for semitone shifts.
     * @param sampleRate   Project sample rate.
     * @return new buffer (length = round(inSamples * timeRatio)), or nullptr.
     */
    static juce::AudioSampleBuffer* rubberbandProcess(const juce::AudioSampleBuffer& input,
                                                      double timeRatio,
                                                      double pitchScale,
                                                      double sampleRate)
    {
        const int numChannels = input.getNumChannels();
        const int inSamples   = input.getNumSamples();
        if (numChannels <= 0 || inSamples <= 0 || sampleRate <= 0.0
            || timeRatio <= 0.0 || pitchScale <= 0.0)
            return nullptr;

        const int outSamples = (int) std::llround((double) inSamples * timeRatio);
        if (outSamples <= 0)
            return nullptr;

        // Identity-Fall: weder Stretch noch Shift — direkte Kopie.
        if (std::abs(timeRatio - 1.0) < 1e-6 && std::abs(pitchScale - 1.0) < 1e-6)
        {
            auto* out = new juce::AudioSampleBuffer(numChannels, inSamples);
            for (int ch = 0; ch < numChannels; ++ch)
                out->copyFrom(ch, 0, input, ch, 0, inSamples);
            return out;
        }

        // Tuning kommt aus den App-Settings — Block/Intervall als Anteil der
        // Sample-Rate (signalsmith-Preset waere 0.12/0.03), Formant-Erhalt nur
        // bei echtem Pitch-Shift relevant.
        const auto* s = Settings::getInstance();
        const double blockPct    = s->getStretchBlockPercent();
        const double intervalPct = s->getStretchIntervalPercent();
        const bool   preserveFormants = s->getStretchFormantPreservation();

        signalsmith::stretch::SignalsmithStretch<float> stretch;
        stretch.configure(numChannels,
                          (int) std::lround(sampleRate * blockPct),
                          (int) std::lround(sampleRate * intervalPct));
        if (std::abs(pitchScale - 1.0) > 1e-9)
        {
            stretch.setTransposeFactor((float) pitchScale);
            if (preserveFormants)
                stretch.setFormantSemitones(0.0f, true);
        }

        std::unique_ptr<juce::AudioSampleBuffer> out(new juce::AudioSampleBuffer(numChannels, outSamples));
        out->clear();

        // signalsmith-stretch ist eine Streaming-Engine mit interner Latenz:
        // beim ersten process()-Aufruf ist der Output um outputLatency()
        // Samples gegenueber dem Input verzoegert. Wuerden wir naiv
        // process(in, N, out, M) aufrufen, waeren die ersten L Output-Samples
        // Ramp-Up-Stille — das ganze Audio im Buffer schoebe sich um L
        // Samples nach hinten und das Timing relativ zur Region waere weg.
        //
        // Sauberer Offline-Trick: wir rendern in einen *groesseren* Temp-Out
        // (M + L Samples) und fuettern dafuer einen entsprechend gepaddeten
        // Input (N + L/timeRatio Samples, Tail = Stille). Damit bleibt die
        // implizite Stretch-Ratio M/N erhalten, und wir koennen die ersten L
        // Output-Samples einfach verwerfen — uebrig bleibt timing-korrektes
        // Audio von Sample 0 an.
        //
        // WICHTIG: die Lib trennt analysisLatency (inputLatency(), in Input-
        // Samples) und synthesisLatency (outputLatency(), in Output-Samples).
        // Trimmt man nur synthesisLatency, bleibt ein Rest-Versatz von
        // analysisLatency * timeRatio Output-Samples uebrig — am Anfang als
        // kleine Stille-Luecke hoerbar. Wir rechnen beide Anteile zusammen.
        const int outLat       = stretch.outputLatency();
        const int inLat        = stretch.inputLatency();
        const int leadOffset   = outLat + (int) std::ceil((double) inLat * timeRatio);
        const int extraInput   = (int) std::ceil((double) leadOffset / timeRatio);
        const int paddedIn     = inSamples  + extraInput;
        const int paddedOutLen = outSamples + leadOffset;

        std::vector<std::vector<float>> padIn ((size_t) numChannels);
        std::vector<std::vector<float>> tmpOut((size_t) numChannels);
        std::vector<const float*> inPtrs ((size_t) numChannels, nullptr);
        std::vector<float*>       outPtrs((size_t) numChannels, nullptr);
        for (int ch = 0; ch < numChannels; ++ch)
        {
            padIn[(size_t) ch].assign((size_t) paddedIn, 0.0f);
            std::copy(input.getReadPointer(ch),
                      input.getReadPointer(ch) + inSamples,
                      padIn[(size_t) ch].data());
            inPtrs[(size_t) ch] = padIn[(size_t) ch].data();

            tmpOut[(size_t) ch].assign((size_t) paddedOutLen, 0.0f);
            outPtrs[(size_t) ch] = tmpOut[(size_t) ch].data();
        }

        stretch.process(inPtrs.data(), paddedIn, outPtrs.data(), paddedOutLen);

        // Latenz-Offset abschneiden — die ersten leadOffset Samples sind
        // Algorithmus-Priming, das eigentliche Audio beginnt bei Index
        // leadOffset.
        for (int ch = 0; ch < numChannels; ++ch)
            out->copyFrom(ch, 0, tmpOut[(size_t) ch].data() + leadOffset, outSamples);

        return out.release();
    }

    /**
     * Time-stretch an audio buffer, pitch preserved.
     * @param timeRatio 2.0 = twice as long, 0.5 = half as long, 1.0 = copy.
     */
    static juce::AudioSampleBuffer* stretchBufferRubberband(const juce::AudioSampleBuffer& input,
                                                            double timeRatio,
                                                            double sampleRate)
    {
        return rubberbandProcess(input, timeRatio, 1.0, sampleRate);
    }

    /**
     * Pitch-shift an audio buffer, time preserved.
     * @param semitones positive = up, negative = down. 12 = one octave.
     */
    static juce::AudioSampleBuffer* pitchShiftBufferRubberband(const juce::AudioSampleBuffer& input,
                                                                double semitones,
                                                                double sampleRate)
    {
        const double pitchScale = std::pow(2.0, semitones / 12.0);
        return rubberbandProcess(input, 1.0, pitchScale, sampleRate);
    }

    /**
     * Reverse the audio buffer by swapping samples from beginning to end
     *
     * @param buffer The audio buffer to reverse
     */
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

    /**
     * Normalize the audio buffer to the maximum possible level (1.0f)
     * Finds the highest absolute value across all channels and scales accordingly
     *
     * @param buffer The audio buffer to normalize
     */
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

    /**
     * Normalize the audio buffer to a specific target level
     *
     * @param buffer The audio buffer to normalize
     * @param targetLevel The target level to normalize to (default: 0.9f)
     */
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

    /**
     * Normalize the audio buffer based on RMS (Root Mean Square) level
     * This provides more perceptually accurate loudness normalization
     *
     * @param buffer The audio buffer to normalize
     * @param targetRMS The target RMS level (default: 0.2f)
     */
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

    /**
     * Normalize each channel individually to its own maximum level
     * This can help balance channels but may alter stereo imaging
     *
     * @param buffer The audio buffer to normalize
     */
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

    // ==================== NEUE FUNKTIONEN ====================

    /**
     * Insert silence at the beginning of the audio buffer
     *
     * @param buffer The audio buffer to modify
     * @param silenceSamples The number of silent samples to insert at the beginning
     */
    static void insertSilenceAtBeginning(AudioSampleBuffer& buffer, int silenceSamples)
    {
        if (silenceSamples <= 0)
            return;

        const int originalSamples = buffer.getNumSamples();
        const int newSamples = originalSamples + silenceSamples;

        // Create a new buffer with increased size
        AudioSampleBuffer newBuffer(buffer.getNumChannels(), newSamples);

        // Clear the beginning (silence)
        newBuffer.clear(0, silenceSamples);

        // Copy original audio data after the silence
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            newBuffer.copyFrom(channel, silenceSamples, buffer, channel, 0, originalSamples);
        }

        // Replace original buffer content
        buffer.setSize(newBuffer.getNumChannels(), newBuffer.getNumSamples(), false, false, true);
		buffer.copyFrom(0, 0, newBuffer, 0, 0, newBuffer.getNumSamples());
    }

    /**
     * Insert silence at the end of the audio buffer
     *
     * @param buffer The audio buffer to modify
     * @param silenceSamples The number of silent samples to append
     */
    static void insertSilenceAtEnd(AudioSampleBuffer& buffer, int silenceSamples)
    {
        if (silenceSamples <= 0)
            return;

        const int originalSamples = buffer.getNumSamples();

        // Resize buffer to accommodate additional silence
        buffer.setSize(buffer.getNumChannels(), originalSamples + silenceSamples, true, true, false);

        // Clear the added samples (they should already be zero, but just to be sure)
        buffer.clear(originalSamples, silenceSamples);
    }

    /**
     * Replace a section of the audio buffer with silence
     *
     * @param buffer The audio buffer to modify
     * @param startSample The starting sample index for the silence
     * @param numSamples The number of samples to silence
     */
    static void applySilence(AudioSampleBuffer& buffer, int startSample, int numSamples)
    {
        if (startSample < 0 || numSamples <= 0 || startSample + numSamples > buffer.getNumSamples())
            return;

        buffer.clear(startSample, numSamples);
    }

    /**
     * Trim the audio buffer from the beginning
     *
     * @param buffer The audio buffer to trim
     * @param samplesToRemove The number of samples to remove from the beginning
     */
    static void trimFromBeginning(AudioSampleBuffer& buffer, int samplesToRemove)
    {
        if (samplesToRemove <= 0 || samplesToRemove >= buffer.getNumSamples())
            return;

        const int newSize = buffer.getNumSamples() - samplesToRemove;
        AudioSampleBuffer newBuffer(buffer.getNumChannels(), newSize);

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            newBuffer.copyFrom(channel, 0, buffer, channel, samplesToRemove, newSize);
        }

        buffer.setSize(newBuffer.getNumChannels(), newBuffer.getNumSamples(), false, false, true);
        buffer.copyFrom(0, 0, newBuffer, 0, 0, newBuffer.getNumSamples());
    }

    /**
     * Trim the audio buffer from the end
     *
     * @param buffer The audio buffer to trim
     * @param samplesToRemove The number of samples to remove from the end
     */
    static void trimFromEnd(AudioSampleBuffer& buffer, int samplesToRemove)
    {
        if (samplesToRemove <= 0 || samplesToRemove >= buffer.getNumSamples())
            return;

        const int newSize = buffer.getNumSamples() - samplesToRemove;
        buffer.setSize(buffer.getNumChannels(), newSize, true, true, false);
    }

    /**
     * Trim silence from the beginning of the audio buffer based on a threshold
     *
     * @param buffer The audio buffer to trim
     * @param threshold The amplitude threshold below which samples are considered silent
     */
    static void trimSilenceFromBeginning(AudioSampleBuffer& buffer, float threshold = 0.001f)
    {
        int samplesToRemove = 0;
        const int numSamples = buffer.getNumSamples();

        // Find first non-silent sample
        for (int sample = 0; sample < numSamples; ++sample)
        {
            bool isSilent = true;

            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            {
                if (std::abs(buffer.getSample(channel, sample)) > threshold)
                {
                    isSilent = false;
                    break;
                }
            }

            if (!isSilent)
                break;

            samplesToRemove++;
        }

        if (samplesToRemove > 0)
            trimFromBeginning(buffer, samplesToRemove);
    }

    /**
     * Trim silence from the end of the audio buffer based on a threshold
     *
     * @param buffer The audio buffer to trim
     * @param threshold The amplitude threshold below which samples are considered silent
     */
    static void trimSilenceFromEnd(AudioSampleBuffer& buffer, float threshold = 0.001f)
    {
        int samplesToRemove = 0;
        const int numSamples = buffer.getNumSamples();

        // Find last non-silent sample
        for (int sample = numSamples - 1; sample >= 0; --sample)
        {
            bool isSilent = true;

            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            {
                if (std::abs(buffer.getSample(channel, sample)) > threshold)
                {
                    isSilent = false;
                    break;
                }
            }

            if (!isSilent)
                break;

            samplesToRemove++;
        }

        if (samplesToRemove > 0)
            trimFromEnd(buffer, samplesToRemove);
    }

    /**
     * Apply a simple pitch shift to the audio buffer using linear interpolation
     * Note: This is a basic implementation. For better quality, consider using more advanced algorithms
     *
     * @param buffer The audio buffer to process
     * @param pitchRatio The pitch ratio (2.0 = one octave up, 0.5 = one octave down)
     */
    static void applySimplePitchShift(AudioSampleBuffer& buffer, float pitchRatio)
    {
        if (pitchRatio <= 0.0f || pitchRatio == 1.0f)
            return;

        const int originalSamples = buffer.getNumSamples();
        const int newSamples = static_cast<int>(originalSamples / pitchRatio);

        AudioSampleBuffer newBuffer(buffer.getNumChannels(), newSamples);

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            const float* sourceData = buffer.getReadPointer(channel);
            float* destData = newBuffer.getWritePointer(channel);

            for (int i = 0; i < newSamples; ++i)
            {
                float sourceIndex = i * pitchRatio;
                int index1 = static_cast<int>(sourceIndex);
                int index2 = index1 + 1;

                if (index1 >= 0 && index2 < originalSamples)
                {
                    float fraction = sourceIndex - index1;
                    destData[i] = sourceData[index1] * (1.0f - fraction) + sourceData[index2] * fraction;
                }
                else if (index1 >= 0 && index1 < originalSamples)
                {
                    destData[i] = sourceData[index1];
                }
                else
                {
                    destData[i] = 0.0f;
                }
            }
        }

        buffer.setSize(newBuffer.getNumChannels(), newBuffer.getNumSamples(), false, false, true);
        buffer.copyFrom(0, 0, newBuffer, 0, 0, newBuffer.getNumSamples());
    }

    /**
     * Apply time stretching to the audio buffer without changing pitch
     * Note: This is a basic implementation using simple overlap-add
     *
     * @param buffer The audio buffer to process
     * @param timeRatio The time stretch ratio (2.0 = twice as long, 0.5 = half as long)
     */
    static void applyTimeStretch(AudioSampleBuffer& buffer, float timeRatio)
    {
        if (timeRatio <= 0.0f || timeRatio == 1.0f)
            return;

        const int originalSamples = buffer.getNumSamples();
        const int newSamples = static_cast<int>(originalSamples * timeRatio);
        const int frameSize = 1024;
        const int hopSize = frameSize / 4;

        AudioSampleBuffer newBuffer(buffer.getNumChannels(), newSamples);
        newBuffer.clear();

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            const float* sourceData = buffer.getReadPointer(channel);
            float* destData = newBuffer.getWritePointer(channel);

            float sourcePos = 0.0f;
            int destPos = 0;

            while (sourcePos + frameSize < originalSamples && destPos + frameSize < newSamples)
            {
                // Simple overlap-add
                for (int i = 0; i < frameSize; ++i)
                {
                    int srcIndex = static_cast<int>(sourcePos) + i;
                    if (srcIndex < originalSamples && destPos + i < newSamples)
                    {
                        destData[destPos + i] += sourceData[srcIndex] * 0.5f;
                    }
                }

                sourcePos += hopSize / timeRatio;
                destPos += hopSize;
            }
        }

        buffer.setSize(newBuffer.getNumChannels(), newBuffer.getNumSamples(), false, false, true);
        buffer.copyFrom(0, 0, newBuffer, 0, 0, newBuffer.getNumSamples());
    }

    /**
     * Apply a DC offset removal filter to the audio buffer
     *
     * @param buffer The audio buffer to process
     */
    static void removeDCOffset(AudioSampleBuffer& buffer)
    {
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            float* channelData = buffer.getWritePointer(channel);
            const int numSamples = buffer.getNumSamples();

            // Calculate the DC offset (average value)
            float sum = 0.0f;
            for (int i = 0; i < numSamples; ++i)
            {
                sum += channelData[i];
            }
            float dcOffset = sum / numSamples;

            // Remove the DC offset
            for (int i = 0; i < numSamples; ++i)
            {
                channelData[i] -= dcOffset;
            }
        }
    }

    /**
     * Apply a simple low-pass filter to the audio buffer
     *
     * @param buffer The audio buffer to process
     * @param cutoffFreq The cutoff frequency in Hz
     * @param sampleRate The sample rate of the audio
     */
    static void applyLowPassFilter(AudioSampleBuffer& buffer, float cutoffFreq, float sampleRate)
    {
        const float rc = 1.0f / (2.0f * juce::MathConstants<float>::pi * cutoffFreq);
        const float dt = 1.0f / sampleRate;
        const float alpha = dt / (rc + dt);

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            float* channelData = buffer.getWritePointer(channel);
            float previousOutput = 0.0f;

            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                previousOutput = previousOutput + alpha * (channelData[i] - previousOutput);
                channelData[i] = previousOutput;
            }
        }
    }

    /**
     * Apply a simple high-pass filter to the audio buffer
     *
     * @param buffer The audio buffer to process
     * @param cutoffFreq The cutoff frequency in Hz
     * @param sampleRate The sample rate of the audio
     */
    static void applyHighPassFilter(AudioSampleBuffer& buffer, float cutoffFreq, float sampleRate)
    {
        const float rc = 1.0f / (2.0f * juce::MathConstants<float>::pi * cutoffFreq);
        const float dt = 1.0f / sampleRate;
        const float alpha = rc / (rc + dt);

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            float* channelData = buffer.getWritePointer(channel);
            float previousInput = 0.0f;
            float previousOutput = 0.0f;

            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                float currentInput = channelData[i];
                float currentOutput = alpha * (previousOutput + currentInput - previousInput);
                channelData[i] = currentOutput;

                previousInput = currentInput;
                previousOutput = currentOutput;
            }
        }
    }

    /**
     * Apply a simple delay effect to the audio buffer
     *
     * @param buffer The audio buffer to process
     * @param delaySamples The delay time in samples
     * @param feedback The feedback amount (0.0 to 0.99)
     * @param mix The wet/dry mix (0.0 = dry, 1.0 = wet)
     */
    static void applyDelay(AudioSampleBuffer& buffer, int delaySamples, float feedback = 0.3f, float mix = 0.3f)
    {
        if (delaySamples <= 0 || delaySamples >= buffer.getNumSamples())
            return;

        feedback = jlimit(0.0f, 0.99f, feedback);
        mix = jlimit(0.0f, 1.0f, mix);

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            float* channelData = buffer.getWritePointer(channel);
            const int numSamples = buffer.getNumSamples();

            // Create a delay buffer
            std::vector<float> delayBuffer(numSamples, 0.0f);

            for (int i = 0; i < numSamples; ++i)
            {
                float input = channelData[i];
                float delayedSample = 0.0f;

                if (i >= delaySamples)
                {
                    delayedSample = delayBuffer[i - delaySamples];
                }

                delayBuffer[i] = input + delayedSample * feedback;
                channelData[i] = input * (1.0f - mix) + delayedSample * mix;
            }
        }
    }

    /**
     * Apply a simple distortion effect to the audio buffer
     *
     * @param buffer The audio buffer to process
     * @param drive The distortion amount (1.0 = no distortion, higher values = more distortion)
     */
    static void applyDistortion(AudioSampleBuffer& buffer, float drive = 2.0f)
    {
        drive = jmax(1.0f, drive);

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            float* channelData = buffer.getWritePointer(channel);

            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                float input = channelData[i] * drive;

                // Soft clipping using tanh
                channelData[i] = std::tanh(input) / drive;
            }
        }
    }

    /**
     * Mix two audio buffers together
     *
     * @param buffer1 The first audio buffer (will be modified)
     * @param buffer2 The second audio buffer to mix in
     * @param gain1 The gain for the first buffer
     * @param gain2 The gain for the second buffer
     */
    static void mixBuffers(AudioSampleBuffer& buffer1, const AudioSampleBuffer& buffer2, float gain1 = 1.0f, float gain2 = 1.0f)
    {
        const int minChannels = jmin(buffer1.getNumChannels(), buffer2.getNumChannels());
        const int minSamples = jmin(buffer1.getNumSamples(), buffer2.getNumSamples());

        for (int channel = 0; channel < minChannels; ++channel)
        {
            float* data1 = buffer1.getWritePointer(channel);
            const float* data2 = buffer2.getReadPointer(channel);

            for (int i = 0; i < minSamples; ++i)
            {
                data1[i] = data1[i] * gain1 + data2[i] * gain2;
            }
        }
    }
}