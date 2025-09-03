#include "Metronome.h"
#include "TransportDisplayComponent.h"
#include "Project/Project.h"

Metronome::Metronome(double sr, int bs)
    : sampleRate(sr), blockSize(bs)
{
    setBPM(bpm);
    setClickFrequency(clickFreq);
    setAccentFrequency(accentFreq);
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

void Metronome::setAccentFrequency(float freqHz)
{
    accentFreq = freqHz;
    accentPhaseDelta = juce::MathConstants<float>::twoPi * accentFreq / static_cast<float>(sampleRate);
}

void Metronome::setClickLengthSamples(int samples)
{
    clickLength = samples;
}

void Metronome::setTimeSignature(int beatsPerBar)
{
    this->beatsPerBar = beatsPerBar;
}

void Metronome::reset()
{
    sampleCounter = 0;
    clickSamplePos = -1;
    beatInBar = 0;
    phase = 0.0f;
}

void Metronome::generateBlock(juce::AudioBuffer<float>& outBuffer)
{
    outBuffer.setSize(2, blockSize, false, false, true);
    outBuffer.clear();

    for (int i = 0; i < blockSize; ++i)
    {
        float sample = 0.0f;

        // Prüfe, ob neuer Beat beginnt
        if (sampleCounter % samplesPerBeat == 0)
        {
            clickSamplePos = 0;
            phase = 0.0f;
            // beatInBar läuft von 0 bis beatsPerBar-1
        }

        // Generiere Click-Sound (basierend auf dem Original)
        if (clickSamplePos >= 0 && clickSamplePos < clickLength)
        {
            // Erster Beat im Takt (beatInBar == 0) bekommt tiefere Frequenz
            if (beatInBar == 0)
            {
                sample = 0.4f * std::sin(phase);  // Etwas lauter für Betonung
                phase += accentPhaseDelta;        // Tiefere Frequenz
            }
            else
            {
                sample = 0.3f * std::sin(phase);
                phase += phaseDelta;              // Normale Frequenz
            }

            clickSamplePos++;

            // Wenn Click zu Ende, nächsten Beat im Takt vorbereiten
            if (clickSamplePos >= clickLength)
            {
                beatInBar = (beatInBar + 1) % beatsPerBar;
            }
        }

        outBuffer.setSample(0, i, sample);
        outBuffer.setSample(1, i, sample);
        sampleCounter++;
    }
}
void Metronome::prepareToPlay(double sampleRate, int blockSize) {
	this->sampleRate = sampleRate;
	this->blockSize = blockSize;
}

void Metronome::changeListenerCallback(ChangeBroadcaster* source)
{
    if (dynamic_cast<TransportDisplayComponent*>(source) != nullptr) {
        TransportDisplayComponent* tl = dynamic_cast<TransportDisplayComponent*>(source);
        setBPM(tl->getCurrentBpm());
	}
}
