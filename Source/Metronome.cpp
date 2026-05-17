#include "Metronome.h"
#include "TransportDisplayComponent.h"
#include "Project/Project.h"
#include "MessageBus/FastBus.h"

Metronome::Metronome(double sr, int bs)
    : sampleRate(sr), blockSize(bs),
    clickSamplePos(-1), beatInBar(0), phase(0.0f),
    beatsPerBar(4), // Standard 4/4 Takt
    clickFreq(1000.0f), accentFreq(800.0f), clickLength(4410), // Standard-Werte
    phaseDelta(0.0f), accentPhaseDelta(0.0f)
{
    setBPM(Project::getInstance()->getTempo());
    setClickFrequency(clickFreq);
    setAccentFrequency(accentFreq);
    setClickLengthSamples(clickLength);

    auto& bus = FastBus::getInstance();

    bus.subscribe<float>("tempo", [this](const FastTopic<float>& topic) {
        this->setBPM(topic.getValue());
    });
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
    // Nur den laufenden Click-Envelope abklemmen. Das Beat-Raster selbst
    // wird per Song-Position in generateBlock berechnet — da gibt es keinen
    // internen Counter mehr, der reset werden muesste.
    clickSamplePos = -1;
    beatInBar = 0;
    phase = 0.0f;
}

void Metronome::generateBlock(juce::AudioBuffer<float>& outBuffer, long blockStartSample)
{
    outBuffer.setSize(2, blockSize, false, false, true);
    outBuffer.clear();

    for (int i = 0; i < blockSize; ++i)
    {
        float sample = 0.0f;
        const long absSample = blockStartSample + (long)i;

        // Beat-Boundary aus der absoluten Song-Position ableiten — dadurch
        // resyncht das Metronom automatisch nach jedem Seek (Track-Click,
        // Piano-Roll-Click, Loop-Wrap, Record-Start an beliebiger Position).
        if (samplesPerBeat > 0 && absSample >= 0 && (absSample % samplesPerBeat) == 0)
        {
            clickSamplePos = 0;
            phase = 0.0f;
            beatInBar = (int)((absSample / samplesPerBeat) % beatsPerBar);
        }

        // Generiere Click-Sound nur f�r kurze Dauer
        if (clickSamplePos >= 0 && clickSamplePos < clickLength)
        {
            if (beatInBar == 0)
            {
                sample = 0.4f * std::sin(phase);
                phase += accentPhaseDelta;
            }
            else
            {
                sample = 0.3f * std::sin(phase);
                phase += phaseDelta;
            }

            clickSamplePos++;
        }
        else if (clickSamplePos >= clickLength)
        {
            clickSamplePos = -1; // Explizit beenden
        }

        outBuffer.setSample(0, i, sample);
        outBuffer.setSample(1, i, sample);
    }
}
void Metronome::prepareToPlay(double sampleRate, int blockSize) {
	this->sampleRate = sampleRate;
	this->blockSize = blockSize;
}
