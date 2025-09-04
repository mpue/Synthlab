#pragma once
#include <JuceHeader.h>

class Metronome 
{
public:
    Metronome(double sampleRate = 44100.0, int blockSize = 128);
    void setBPM(float newBPM);
    void setClickFrequency(float freqHz);
    void setAccentFrequency(float freqHz);
    void setClickLengthSamples(int samples);
    void setTimeSignature(int beatsPerBar);
    void reset();
    void generateBlock(juce::AudioBuffer<float>& outBuffer);
	void prepareToPlay(double sampleRate, int blockSize);

private:
    double sampleRate;
    int blockSize;
    float bpm = 120.0f;
    float clickFreq = 1200.0f;      // Höhere Frequenz für normalen Click
    float accentFreq = 800.0f;      // Tiefere Frequenz für betonten Click
    int clickLength = 2205;
    int beatsPerBar = 4;
    int samplesPerBeat = 0;
    int sampleCounter = 0;
    int clickSamplePos = -1;
    int beatInBar = 0;              // Welcher Beat im Takt (0-3 bei 4/4)
    float phase = 0.0f;
    float phaseDelta = 0.0f;
    float accentPhaseDelta = 0.0f;
};