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
    // blockStartSample = absolute Song-Sample-Position am Block-Anfang.
    // Treibt das Beat-Raster: das Metronom kennt keinen eigenen Counter mehr,
    // sondern leitet jeden Tick aus der globalen Position ab — damit
    // resyncht es automatisch bei Seek (Klick auf Track, Loop-Wrap, etc.).
    void generateBlock(juce::AudioBuffer<float>& outBuffer, long blockStartSample);
	void prepareToPlay(double sampleRate, int blockSize);

private:
    double sampleRate;
    int blockSize;
    float bpm = 120.0f;
    float clickFreq = 1200.0f;      // H�here Frequenz f�r normalen Click
    float accentFreq = 800.0f;      // Tiefere Frequenz f�r betonten Click
    int clickLength = 2205;
    int beatsPerBar = 4;
    int samplesPerBeat = 0;
    int clickSamplePos = -1;        // Fortschritt im aktuellen Click-Envelope
    int beatInBar = 0;              // Welcher Beat im Takt (0-3 bei 4/4)
    float phase = 0.0f;
    float phaseDelta = 0.0f;
    float accentPhaseDelta = 0.0f;
};