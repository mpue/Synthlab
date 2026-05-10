/*
  ==============================================================================

    SuperSawModule.cpp

    Detune-Curve und Mix-Curve sind an die JP-8000-Supersaw angelehnt
    (Adam Szabo "How to emulate the Super Saw"). Wir nehmen aber eine
    leicht vereinfachte Form — der Charakter steht und wir sparen uns die
    Polynome im Audio-Pfad indem wir alle Voice-Parameter nur dann neu
    rechnen wenn sich Detune/Mix/Width geaendert haben.

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "SuperSawModule.h"
#include "Connection.h"
#include <cmath>

using juce::Justification;
using juce::PropertyComponent;
using juce::BooleanPropertyComponent;
using juce::Value;

SuperSawModule::SuperSawModule(double sampleRate, int buffersize)
{
    this->sampleRate = sampleRate;
    this->buffersize = buffersize;

    for (int v = 0; v < kNumVoices; ++v)
        monoVoice[v] = new Sawtooth((float) sampleRate, buffersize);

    for (int i = 0; i < 128; ++i) {
        for (int v = 0; v < kNumVoices; ++v) {
            polyVoice[i][v] = new Sawtooth((float) sampleRate, buffersize);
            polyVoice[i][v]->setVolume(0.0f);
        }
        voiceActive[i] = false;
    }

    setSize(120, 140);
    nameLabel->setJustificationType(Justification::left);
    nameLabel->setTopLeftPosition(18, 2);
    setName("SuperSaw");

    editable = false;
    prefab = true;

    recomputeVoiceMix();
}

SuperSawModule::~SuperSawModule()
{
    for (int v = 0; v < kNumVoices; ++v) {
        delete monoVoice[v];
        monoVoice[v] = nullptr;
    }
    for (int i = 0; i < 128; ++i) {
        for (int v = 0; v < kNumVoices; ++v) {
            delete polyVoice[i][v];
            polyVoice[i][v] = nullptr;
        }
    }
    delete isMonoListener;
    delete isMonoValue;
}

void SuperSawModule::configurePins() {
    Pin* p = new Pin(Pin::Type::VALUE);
    p->direction = Pin::Direction::IN;
    p->listeners.push_back(this);
    p->setName("P");
    p->setDescription("Pitch-Transpose in Halbtoenen.");

    Pin* f = new Pin(Pin::Type::VALUE);
    f->direction = Pin::Direction::IN;
    f->listeners.push_back(this);
    f->setName("F");
    f->setDescription("Fine in Hz, additiv auf die Grundfrequenz.");

    Pin* a = new Pin(Pin::Type::VALUE);
    a->direction = Pin::Direction::IN;
    a->listeners.push_back(this);
    a->setName("A");
    a->setDescription("Gate-/Volume-Pin (z.B. von MIDI-Gate oder ADSR).");

    Pin* d = new Pin(Pin::Type::VALUE);
    d->direction = Pin::Direction::IN;
    d->listeners.push_back(this);
    d->setName("D");
    d->setDescription("Detune 0..1 -> 0..50 Cent Spread zwischen den 7 Stimmen.");

    Pin* m = new Pin(Pin::Type::VALUE);
    m->direction = Pin::Direction::IN;
    m->listeners.push_back(this);
    m->setName("M");
    m->setDescription("Mix 0..1. Center vs Side-Voices. 0=nur Center, 1=nur Sides.");

    Pin* w = new Pin(Pin::Type::VALUE);
    w->direction = Pin::Direction::IN;
    w->listeners.push_back(this);
    w->setName("W");
    w->setDescription("Stereo-Width 0..1. 0=Mono, 1=volle Auffaecherung.");

    Pin* outLp = new Pin(Pin::Type::AUDIO);
    outLp->direction = Pin::Direction::OUT;
    outLp->listeners.push_back(this);
    outLp->setName("L");

    Pin* outRp = new Pin(Pin::Type::AUDIO);
    outRp->direction = Pin::Direction::OUT;
    outRp->listeners.push_back(this);
    outRp->setName("R");

    Pin* vMon = new Pin(Pin::Type::VALUE);
    vMon->direction = Pin::Direction::OUT;
    vMon->listeners.push_back(this);
    vMon->setName("V");
    vMon->setDescription("Peak-Monitor (Block-letzter Wert, mono-summiert).");

    addPin(Pin::Direction::IN,  p);
    addPin(Pin::Direction::IN,  f);
    addPin(Pin::Direction::IN,  a);
    addPin(Pin::Direction::IN,  d);
    addPin(Pin::Direction::IN,  m);
    addPin(Pin::Direction::IN,  w);
    addPin(Pin::Direction::OUT, outLp);
    addPin(Pin::Direction::OUT, outRp);
    addPin(Pin::Direction::OUT, vMon);

    createProperties();

    outL = pins.at(6)->getAudioBuffer();
    outR = pins.at(7)->getAudioBuffer();
}

void SuperSawModule::paint(juce::Graphics& g) {
    Module::paint(g);
    g.drawImageAt(juce::ImageCache::getFromMemory(BinaryData::saw_png, BinaryData::saw_pngSize), 25, 40);
}

void SuperSawModule::setMono(bool value) {
    this->mono = value;
    if (this->isMonoValue != nullptr)
        this->isMonoValue->setValue(value);
}

bool SuperSawModule::isMono() {
    return mono;
}

juce::Array<PropertyComponent*>& SuperSawModule::getProperties() {
    properties = juce::Array<PropertyComponent*>();
    isMonoProp = new BooleanPropertyComponent(*isMonoValue, "isMono", "Mono");
    properties.add(isMonoProp);
    return properties;
}

void SuperSawModule::createProperties() {
    isMonoValue = new Value();
    isMonoListener = new IsMonoListener(*isMonoValue, this);
    isMonoValue->setValue(mono);
}

void SuperSawModule::recomputeVoiceMix() {
    // Cent-Spread: voices i=0..6 verteilen sich symmetrisch um die Mitte
    // (Index 3). Maximaler Spread bei detune=1 = +-50 Cent — dasselbe Fenster
    // wie der JP-8000. Lineare Verteilung statt Szabos kubischer Curve, weil
    // wir die Curve-Form schon aus Mix+Pan kriegen.
    const float maxCents = 50.0f;
    for (int i = 0; i < kNumVoices; ++i) {
        const float t = (float)(i - 3) / 3.0f;   // -1..+1 ueber 7 Voices
        voiceCentOffset[i] = t * maxCents * detuneAmount;
    }

    // Mix-Curve nach Szabo (vereinfacht): centerGain faellt mit Mix,
    // sideGain hat Maximum etwa bei Mix=0.7. Damit wandert die
    // Klangfarbe sauber von "single saw" nach "alle 7 fett".
    const float m = juce::jlimit(0.0f, 1.0f, mixAmount);
    const float centerGain = -0.55f * m + 0.7f;          // 0.7 -> 0.15
    const float sideGain   = -0.73f * m * m + 1.03f * m; // 0   -> 0.30 (peak ~m=0.7)

    // Pan-Layout: alterniert L/R fuer die Side-Voices, Center bleibt mittig.
    // (Klassisch JP-8000-aehnlich, aber ohne festes 6-Bit-Muster.)
    static const float panLayout[kNumVoices] = { -1.0f, +1.0f, -1.0f, 0.0f, +1.0f, -1.0f, +1.0f };
    const float w = juce::jlimit(0.0f, 1.0f, widthAmount);

    for (int i = 0; i < kNumVoices; ++i) {
        const bool isCenter = (i == 3);
        const float baseGain = isCenter ? centerGain : sideGain;

        const float pan = panLayout[i] * w;              // -1..+1
        const float angle = (pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi;  // 0..pi/2
        voicePanL[i] = baseGain * std::cos(angle);
        voicePanR[i] = baseGain * std::sin(angle);

        // Gain (mono-summiert) — nur fuer den V-Monitor und Mono-Mode.
        voiceGain[i] = baseGain;
    }
}

void SuperSawModule::updateVoiceFrequencies(int slot, double baseFreq) {
    Sawtooth** voices = (slot < 0) ? monoVoice : polyVoice[slot];
    for (int v = 0; v < kNumVoices; ++v) {
        // Cent -> Frequenzfaktor: 2^(cents/1200)
        const double factor = std::pow(2.0, voiceCentOffset[v] / 1200.0);
        voices[v]->setFrequency(baseFreq * factor + (double) fine);
    }
}

void SuperSawModule::process() {
    if (pins.size() < 9 || sampleRate <= 0.0 || buffersize <= 0) return;

    // CV-Pins lesen. Pitch und Fine wandern nur in den frequenz-update-Zweig
    // wenn sie wirklich gebraucht werden — Frequenzberechnung ist ein pow().
    if (pins.at(0)->getConnections().size() == 1)
        pitch = (int) pins.at(0)->getConnections().at(0)->getValue();

    if (pins.at(1)->getConnections().size() == 1)
        fine = pins.at(1)->getConnections().at(0)->getValue();

    bool mixDirty = false;
    if (pins.at(3)->getConnections().size() == 1) {
        const float d = juce::jlimit(0.0f, 1.0f, pins.at(3)->getConnections().at(0)->getValue());
        if (std::abs(d - detuneAmount) > 1.0e-4f) { detuneAmount = d; mixDirty = true; }
    }
    if (pins.at(4)->getConnections().size() == 1) {
        const float mv = juce::jlimit(0.0f, 1.0f, pins.at(4)->getConnections().at(0)->getValue());
        if (std::abs(mv - mixAmount) > 1.0e-4f) { mixAmount = mv; mixDirty = true; }
    }
    if (pins.at(5)->getConnections().size() == 1) {
        const float wv = juce::jlimit(0.0f, 1.0f, pins.at(5)->getConnections().at(0)->getValue());
        if (std::abs(wv - widthAmount) > 1.0e-4f) { widthAmount = wv; mixDirty = true; }
    }
    if (mixDirty) recomputeVoiceMix();

    const bool gateConnected = pins.at(2)->getConnections().size() == 1;
    if (gateConnected)
        gatePin = pins.at(2)->getConnections().at(0);

    if (!gateConnected || outL == nullptr || outR == nullptr) {
        if (outL != nullptr && outL->getNumChannels() > 0) outL->clear();
        if (outR != nullptr && outR->getNumChannels() > 0) outR->clear();
        return;
    }

    float* writeL = outL->getWritePointer(0);
    float* writeR = outR->getWritePointer(0);

    float lastSampleSum = 0.0f;

    if (mono) {
        // Mono-Mode: ein Voice-Slot, Frequenz aus pitch.
        const double baseFreq = 440.0 * std::pow(2.0, ((double) pitch - 69.0) / 12.0);
        updateVoiceFrequencies(-1, baseFreq);

        const float gateVal = pins.at(2)->getConnections().at(0)->getValue();
        for (int v = 0; v < kNumVoices; ++v)
            monoVoice[v]->setVolume(gateVal);

        for (int i = 0; i < buffersize; ++i) {
            float l = 0.0f, r = 0.0f;
            for (int v = 0; v < kNumVoices; ++v) {
                const float s = monoVoice[v]->process();
                l += s * voicePanL[v];
                r += s * voicePanR[v];
            }
            writeL[i] = l;
            writeR[i] = r;
            lastSampleSum = l + r;
        }
    }
    else {
        // Polyphon: jede aktive MIDI-Note bekommt ihren eigenen 7er-Stack.
        // Zustands-Update aus dem gatePin-Buffer (dataEnabled[] / data[]).
        for (int j = 0; j < 128; ++j) {
            if (gatePin->dataEnabled[j]) {
                voiceActive[j] = true;
                // Re-apply every block so pitch/fine/detune changes reach the
                // sounding voices — including during the envelope release.
                const double baseFreq = 440.0 * std::pow(2.0, ((double)(j + 1 + pitch) - 69.0) / 12.0);
                updateVoiceFrequencies(j, baseFreq);
                for (int v = 0; v < kNumVoices; ++v)
                    polyVoice[j][v]->setVolume(gatePin->data[j]);
            }
            else if (voiceActive[j]) {
                voiceActive[j] = false;
                for (int v = 0; v < kNumVoices; ++v)
                    polyVoice[j][v]->setVolume(0.0f);
            }
        }

        for (int i = 0; i < buffersize; ++i) {
            float l = 0.0f, r = 0.0f;
            for (int j = 0; j < 128; ++j) {
                if (!voiceActive[j]) continue;
                for (int v = 0; v < kNumVoices; ++v) {
                    const float s = polyVoice[j][v]->process();
                    l += s * voicePanL[v];
                    r += s * voicePanR[v];
                }
            }
            writeL[i] = l;
            writeR[i] = r;
            lastSampleSum = l + r;
        }
    }

    // V-Monitor: einfache Block-Endgroesse, mono-summiert. Konsistent mit
    // SawtoothModule, das auch nur den letzten Sample-Wert raushaut.
    pins.at(8)->setValue(std::abs(lastSampleSum));
}
