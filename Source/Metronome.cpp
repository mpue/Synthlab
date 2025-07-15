#include "Metronome.h"

Metronome::Metronome(double sr, int bs)
    : sampleRate(sr), blockSize(bs)
{
    setBPM(bpm);
    setClickFrequency(clickFreq);
    setClickLengthSamples(clickLength);
}

void Metronome::setBPM(float newBPM)
{
    bpm = newBPM;
    samplesPerBeat = static_cast<int>((60.0 / bpm) * sampleRate);
}

void Metronome::setClickFrequency(float freqHz)
{
    clickFreq = freqHz;
    phaseDelta = juce::MathConstants<float>::twoPi * clickFreq / static_cast<float>(sampleRate);
}

void Metronome::setClickLengthSamples(int samples)
{
    clickLength = samples;
}

void Metronome::reset()
{
    sampleCounter = 0;
    clickSamplePos = -1;
    phase = 0.0f;
}

void Metronome::generateBlock(juce::AudioBuffer<float>& outBuffer)
{
    outBuffer.setSize(2, blockSize, false, false, true);
    outBuffer.clear();

    for (int i = 0; i < blockSize; ++i)
    {
        float sample = 0.0f;

        if (sampleCounter % samplesPerBeat == 0)
        {
            clickSamplePos = 0;
            phase = 0.0f;
        }

        if (clickSamplePos >= 0 && clickSamplePos < clickLength)
        {
            sample = 0.3f * std::sin(phase);
            phase += phaseDelta;
            clickSamplePos++;
        }

        outBuffer.setSample(0, i, sample);
        outBuffer.setSample(1, i, sample);

        sampleCounter++;
    }
}
