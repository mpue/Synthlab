#pragma once

#include <JuceHeader.h>

class Metronome
{
public:
    Metronome(double sampleRate = 44100.0, int blockSize = 512);

    void setBPM(float newBPM);
    void setClickFrequency(float freqHz);
    void setClickLengthSamples(int samples);
    void reset();

    void generateBlock(juce::AudioBuffer<float>& outBuffer);

private:
    double sampleRate;
    int blockSize;

    float bpm = 120.0f;
    float clickFreq = 1000.0f;
    int clickLength = 2205;

    int samplesPerBeat = 0;
    int sampleCounter = 0;
    int clickSamplePos = -1;

    float phase = 0.0f;
    float phaseDelta = 0.0f;
};
