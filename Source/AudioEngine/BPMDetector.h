#include "../JuceLibraryCode/JuceHeader.h"

class BPMDetector
{
public:
    float detectBPM(const juce::AudioBuffer<float>& buffer, double sampleRate)
    {
        // 1. Energie-basierte Onset Detection
        juce::Array<int> onsets = detectOnsets(buffer, sampleRate);

        if (onsets.size() < 2)
            return 0.0f;

        // 2. Berechne durchschnittlichen Abstand zwischen Onsets
        float averageInterval = calculateAverageInterval(onsets, sampleRate);

        // 3. Konvertiere zu BPM
        float bpm = 60.0f / averageInterval;

        // Typischer BPM-Bereich: 60-180
        return juce::jlimit(60.0f, 180.0f, bpm);
    }

private:
    juce::Array<int> detectOnsets(const juce::AudioBuffer<float>& buffer,
        double sampleRate)
    {
        juce::Array<int> onsets;
        const int hopSize = 512;
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();

        juce::Array<float> energyValues;

        // Berechne Energie für jeden Frame
        for (int i = 0; i < numSamples - hopSize; i += hopSize)
        {
            float energy = 0.0f;

            for (int ch = 0; ch < numChannels; ++ch)
            {
                const float* channelData = buffer.getReadPointer(ch);

                for (int s = i; s < i + hopSize && s < numSamples; ++s)
                {
                    energy += channelData[s] * channelData[s];
                }
            }

            energyValues.add(energy / (hopSize * numChannels));
        }

        // Finde Peaks in der Energie (= Onsets)
        const float threshold = calculateThreshold(energyValues);

        for (int i = 1; i < energyValues.size() - 1; ++i)
        {
            if (energyValues[i] > threshold &&
                energyValues[i] > energyValues[i - 1] &&
                energyValues[i] > energyValues[i + 1])
            {
                onsets.add(i * hopSize);
            }
        }

        return onsets;
    }

    float calculateThreshold(const juce::Array<float>& values)
    {
        float sum = 0.0f;
        for (auto val : values)
            sum += val;

        float mean = sum / values.size();

        // Threshold = Mittelwert + Standardabweichung
        float variance = 0.0f;
        for (auto val : values)
            variance += (val - mean) * (val - mean);

        float stdDev = std::sqrt(variance / values.size());

        return mean + stdDev * 1.5f;
    }

    float calculateAverageInterval(const juce::Array<int>& onsets,
        double sampleRate)
    {
        if (onsets.size() < 2)
            return 0.0f;

        float totalInterval = 0.0f;

        for (int i = 1; i < onsets.size(); ++i)
        {
            float interval = (onsets[i] - onsets[i - 1]) / (float)sampleRate;
            totalInterval += interval;
        }

        return totalInterval / (onsets.size() - 1);
    }
};