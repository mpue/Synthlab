/*
  ==============================================================================

    LFOModule.cpp

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "LFOModule.h"
#include <cmath>

LFOModule::LFOModule(double sampleRate, int buffersize)
{
    this->sampleRate = sampleRate;
    this->buffersize = buffersize;

    setSize(120, 140);
    setName("LFO");

    editable = false;
    prefab = true;
}

LFOModule::~LFOModule()
{
}

void LFOModule::configurePins() {
    // pins[0] = R (rate Hz)
    // pins[1] = S (shape index 0..5)
    // pins[2] = T (sync trigger, EVENT)
    // pins[3] = Bi (bipolar -1..1)
    // pins[4] = Uni (unipolar 0..1)
    Pin* rate = new Pin(Pin::Type::VALUE);
    rate->direction = Pin::Direction::IN;
    rate->listeners.push_back(this);
    rate->setName("R");
    rate->setDescription("Rate in Hz. Default 1.0 wenn nicht verbunden. Negative Werte werden geclamped.");

    Pin* shape = new Pin(Pin::Type::VALUE);
    shape->direction = Pin::Direction::IN;
    shape->listeners.push_back(this);
    shape->setName("S");
    shape->setDescription("Shape: 0=Sine, 1=Triangle, 2=Saw up, 3=Saw down, 4=Square, 5=Sample&Hold.");

    Pin* trig = new Pin(Pin::Type::EVENT);
    trig->direction = Pin::Direction::IN;
    trig->listeners.push_back(this);
    trig->setName("T");
    trig->setDescription("Sync-Trigger. Setzt die Phase auf 0 (Hard-Sync).");

    Pin* outBi = new Pin(Pin::Type::VALUE);
    outBi->direction = Pin::Direction::OUT;
    outBi->listeners.push_back(this);
    outBi->setName("Bi");
    outBi->setDescription("Bipolarer Output -1..1.");

    Pin* outUni = new Pin(Pin::Type::VALUE);
    outUni->direction = Pin::Direction::OUT;
    outUni->listeners.push_back(this);
    outUni->setName("Uni");
    outUni->setDescription("Unipolarer Output 0..1 (= bipolar * 0.5 + 0.5).");

    addPin(Pin::Direction::IN, rate);
    addPin(Pin::Direction::IN, shape);
    addPin(Pin::Direction::IN, trig);
    addPin(Pin::Direction::OUT, outBi);
    addPin(Pin::Direction::OUT, outUni);
}

void LFOModule::paint(juce::Graphics& g) {
    Module::paint(g);
}

void LFOModule::process() {
    if (pins.size() < 5 || sampleRate <= 0.0 || buffersize <= 0) return;

    float rate = 1.0f;
    if (pins.at(0)->getConnections().size() == 1)
        rate = pins.at(0)->getConnections().at(0)->getValue();
    if (rate < 0.0f) rate = 0.0f;
    if (rate > 200.0f) rate = 200.0f;  // LFO ist Sub-Audio; verhindern dass jemand 20kHz reinkippt

    int shape = static_cast<int>(SINE);
    if (pins.at(1)->getConnections().size() == 1) {
        const float s = pins.at(1)->getConnections().at(0)->getValue();
        shape = juce::jlimit(0, 5, static_cast<int>(std::round(s)));
    }

    const double phaseInc = rate / sampleRate;

    // Phase ueber den ganzen Block fortschreiben — Output wird einmal pro
    // Block am Ende gesetzt (das matcht alle anderen VALUE-Pin-Module). Wir
    // koennten pro Sample sampeln, aber fuer LFO-Frequenzen unterhalb 100 Hz
    // ist das nicht hoerbar relevant; die Block-Granularitaet (5-20 ms) ist
    // niedriger als die LFO-Periode.
    bool wrappedThisBlock = false;
    for (int i = 0; i < buffersize; ++i) {
        phase += phaseInc;
        if (phase >= 1.0) {
            phase -= std::floor(phase);
            wrappedThisBlock = true;
        }
    }

    // Bei jedem neuen Zyklus (oder am ersten Aufruf): neuen S&H-Wert wuerfeln.
    if (shape == SAMPLE_HOLD && (wrappedThisBlock || !shFresh)) {
        shFrozen = uni(rng);
        shFresh = true;
    }

    float bi = 0.0f;
    const double p = phase;  // 0..1

    switch (shape) {
        case SINE:
            bi = static_cast<float>(std::sin(2.0 * juce::MathConstants<double>::pi * p));
            break;
        case TRIANGLE:
            // 0..1 Phase -> 0..1..0 -> *2-1 -> -1..1..-1
            bi = static_cast<float>(1.0 - std::abs(2.0 * p - 1.0) * 2.0);
            // obige Formel ergibt 1 bei p=0.5 und -1 bei p=0/p=1. Passt.
            break;
        case SAW_UP:
            bi = static_cast<float>(2.0 * p - 1.0);
            break;
        case SAW_DOWN:
            bi = static_cast<float>(1.0 - 2.0 * p);
            break;
        case SQUARE:
            bi = (p < 0.5) ? 1.0f : -1.0f;
            break;
        case SAMPLE_HOLD:
            bi = shFrozen;
            break;
    }

    pins.at(3)->setValue(bi);
    pins.at(4)->setValue(bi * 0.5f + 0.5f);
}

void LFOModule::eventReceived(Event* e) {
    if (e == nullptr) return;
    if (e->getType() != Event::Type::GATE) return;
    if (e->getValue() <= 0) return;
    // Hard-Sync: Phase auf 0, S&H-Wert frisch wuerfeln, damit ein Sync sofort
    // einen neuen Zustand sichtbar macht (sonst kommt der erst beim naechsten
    // natuerlichen Wrap).
    phase = 0.0;
    shFrozen = uni(rng);
    shFresh = true;
}
