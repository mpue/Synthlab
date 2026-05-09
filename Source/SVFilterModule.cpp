/*
  ==============================================================================

    SVFilterModule.cpp

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "SVFilterModule.h"
#include <cmath>

SVFilterModule::SVFilterModule(double sampleRate, int buffersize)
{
    this->sampleRate = sampleRate;
    this->buffersize = buffersize;

    setSize(120, 140);
    setName("SVF");

    editable = false;
    prefab = true;

    recomputeCoefficients();
}

SVFilterModule::~SVFilterModule()
{
}

void SVFilterModule::configurePins() {
    // pins[0] = F  (VALUE) frequency Hz
    // pins[1] = Q  (VALUE) resonance
    // pins[2] = Mod (VALUE) multiplikativer Cutoff-Modulator (Envelope o.ae.)
    // pins[3] = In (AUDIO) input
    // pins[4] = LP (AUDIO) lowpass out
    // pins[5] = HP (AUDIO) highpass out
    // pins[6] = BP (AUDIO) bandpass out
    // pins[7] = N  (AUDIO) notch (= LP + HP)
    Pin* f = new Pin(Pin::Type::VALUE);
    f->direction = Pin::Direction::IN;
    f->listeners.push_back(this);
    f->setName("F");
    f->setDescription("Cutoff in Hz. Default 1000 wenn nicht verbunden.");

    Pin* q = new Pin(Pin::Type::VALUE);
    q->direction = Pin::Direction::IN;
    q->listeners.push_back(this);
    q->setName("Q");
    q->setDescription("Resonanz / Q. Default 1.0. Werte 0.5..20 sinnvoll.");

    Pin* modP = new Pin(Pin::Type::VALUE);
    modP->direction = Pin::Direction::IN;
    modP->listeners.push_back(this);
    modP->setName("Mod");
    modP->setDescription("Multiplikativer Cutoff-Modulator. cutoff = F * Mod. Default Mod=1.0.");

    Pin* in = new Pin(Pin::Type::AUDIO);
    in->direction = Pin::Direction::IN;
    in->listeners.push_back(this);
    in->setName("In");
    in->setDescription("Audio in.");

    Pin* lp = new Pin(Pin::Type::AUDIO);
    lp->direction = Pin::Direction::OUT;
    lp->listeners.push_back(this);
    lp->setName("LP");
    lp->setDescription("Lowpass-Out.");

    Pin* hp = new Pin(Pin::Type::AUDIO);
    hp->direction = Pin::Direction::OUT;
    hp->listeners.push_back(this);
    hp->setName("HP");
    hp->setDescription("Highpass-Out.");

    Pin* bp = new Pin(Pin::Type::AUDIO);
    bp->direction = Pin::Direction::OUT;
    bp->listeners.push_back(this);
    bp->setName("BP");
    bp->setDescription("Bandpass-Out.");

    Pin* notch = new Pin(Pin::Type::AUDIO);
    notch->direction = Pin::Direction::OUT;
    notch->listeners.push_back(this);
    notch->setName("N");
    notch->setDescription("Notch-Out (= LP + HP).");

    addPin(Pin::Direction::IN, f);
    addPin(Pin::Direction::IN, q);
    addPin(Pin::Direction::IN, modP);
    addPin(Pin::Direction::IN, in);
    addPin(Pin::Direction::OUT, lp);
    addPin(Pin::Direction::OUT, hp);
    addPin(Pin::Direction::OUT, bp);
    addPin(Pin::Direction::OUT, notch);
}

void SVFilterModule::paint(juce::Graphics& g) {
    Module::paint(g);
}

void SVFilterModule::recomputeCoefficients() {
    if (sampleRate <= 0.0) return;

    // Cutoff aus frequency*mod, mit Floor und Nyquist-Limit.
    double cutoff = static_cast<double>(frequency) * static_cast<double>(mod);
    if (cutoff < 20.0) cutoff = 20.0;
    const double nyq = sampleRate * 0.5 - 1.0;
    if (cutoff > nyq) cutoff = nyq;

    double Q = resonance;
    if (Q < 0.05) Q = 0.05;

    const double g = std::tan(juce::MathConstants<double>::pi * cutoff / sampleRate);
    k = 1.0 / Q;
    a1 = 1.0 / (1.0 + g * (g + k));
    a2 = g * a1;
    a3 = g * a2;
}

void SVFilterModule::process() {
    if (pins.size() < 8 || sampleRate <= 0.0 || buffersize <= 0) return;

    // CV-Pins lesen und ggf. Coefficients neu rechnen. Wir merken uns,
    // ob sich was geaendert hat — Coefficient-Recompute ist nicht gratis.
    bool dirty = false;

    if (pins.at(0)->getConnections().size() == 1) {
        const float v = pins.at(0)->getConnections().at(0)->getValue();
        if (std::abs(v - frequency) > 1.0e-3f) {
            frequency = v;
            dirty = true;
        }
    }
    if (pins.at(1)->getConnections().size() == 1) {
        const float v = pins.at(1)->getConnections().at(0)->getValue();
        if (std::abs(v - resonance) > 1.0e-4f) {
            resonance = v;
            dirty = true;
        }
    }
    if (pins.at(2)->getConnections().size() == 1) {
        const float v = pins.at(2)->getConnections().at(0)->getValue();
        if (std::abs(v - mod) > 1.0e-4f) {
            mod = v;
            dirty = true;
        }
    }
    if (dirty) recomputeCoefficients();

    juce::AudioSampleBuffer* inBuf = nullptr;
    if (pins.at(3)->getConnections().size() >= 1)
        inBuf = pins.at(3)->getConnections().at(0)->getAudioBuffer();

    juce::AudioSampleBuffer* lpBuf = pins.at(4)->getAudioBuffer();
    juce::AudioSampleBuffer* hpBuf = pins.at(5)->getAudioBuffer();
    juce::AudioSampleBuffer* bpBuf = pins.at(6)->getAudioBuffer();
    juce::AudioSampleBuffer* nBuf  = pins.at(7)->getAudioBuffer();

    if (lpBuf == nullptr || hpBuf == nullptr || bpBuf == nullptr || nBuf == nullptr) return;

    if (inBuf == nullptr) {
        // Kein Audio-Input verbunden — alle Outputs auf Null. Ohne diesen
        // Zweig wuerden die Out-Buffer mit Garbage-/Vorblock-Werten dudeln.
        for (int i = 0; i < buffersize; ++i) {
            lpBuf->setSample(0, i, 0.0f);
            hpBuf->setSample(0, i, 0.0f);
            bpBuf->setSample(0, i, 0.0f);
            nBuf->setSample(0, i, 0.0f);
        }
        return;
    }

    const float* in = inBuf->getReadPointer(0);
    float* lpW = lpBuf->getWritePointer(0);
    float* hpW = hpBuf->getWritePointer(0);
    float* bpW = bpBuf->getWritePointer(0);
    float* nW  = nBuf->getWritePointer(0);

    for (int i = 0; i < buffersize; ++i) {
        const double v0 = static_cast<double>(in[i]);
        const double v3 = v0 - ic2eq;
        const double v1 = a1 * ic1eq + a2 * v3;
        const double v2 = ic2eq + a2 * ic1eq + a3 * v3;
        ic1eq = 2.0 * v1 - ic1eq;
        ic2eq = 2.0 * v2 - ic2eq;

        const double lp = v2;
        const double bp = v1;
        const double hp = v0 - k * v1 - v2;
        // Notch = LP + HP (= v0 - k*v1)
        const double notch = lp + hp;

        lpW[i] = static_cast<float>(lp);
        hpW[i] = static_cast<float>(hp);
        bpW[i] = static_cast<float>(bp);
        nW[i]  = static_cast<float>(notch);
    }
}
