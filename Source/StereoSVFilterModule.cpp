/*
  ==============================================================================

    StereoSVFilterModule.cpp

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "StereoSVFilterModule.h"
#include "Connection.h"
#include <cmath>

using juce::Value;
using juce::PropertyComponent;
using juce::BooleanPropertyComponent;
using juce::SliderPropertyComponent;

StereoSVFilterModule::StereoSVFilterModule(double sampleRate, int buffersize)
{
    this->sampleRate = sampleRate;
    this->buffersize = buffersize;

    setSize(140, 200);          // mehr Hoehe wegen 14 Pins
    setName("Stereo SVF");

    editable = false;
    prefab = true;

    recomputeCoefficients();
}

StereoSVFilterModule::~StereoSVFilterModule()
{
    delete slope24Listener;
    delete slope24Value;
    delete spreadListener;
    delete spreadValue;
}

void StereoSVFilterModule::configurePins() {
    // pins[0..5]  = IN: InL, InR, F, Q, Mod, Drive
    // pins[6..13] = OUT: LP_L, LP_R, HP_L, HP_R, BP_L, BP_R, N_L, N_R
    auto makeIn = [&](Pin::Type t, const juce::String& n, const juce::String& d) {
        Pin* p = new Pin(t);
        p->direction = Pin::Direction::IN;
        p->listeners.push_back(this);
        p->setName(n);
        p->setDescription(d);
        addPin(Pin::Direction::IN, p);
    };
    auto makeOut = [&](const juce::String& n, const juce::String& d) {
        Pin* p = new Pin(Pin::Type::AUDIO);
        p->direction = Pin::Direction::OUT;
        p->listeners.push_back(this);
        p->setName(n);
        p->setDescription(d);
        addPin(Pin::Direction::OUT, p);
    };

    makeIn(Pin::Type::AUDIO, "InL",   "Audio-Eingang Links.");
    makeIn(Pin::Type::AUDIO, "InR",   "Audio-Eingang Rechts.");
    makeIn(Pin::Type::VALUE, "F",     "Cutoff in Hz. Default 1000 wenn nicht verbunden.");
    makeIn(Pin::Type::VALUE, "Q",     "Resonanz / Q. 0.5..20 sinnvoll, hoch -> Selbst-Oszillation.");
    makeIn(Pin::Type::VALUE, "Mod",   "Multiplikativer Cutoff-Modulator. cutoff = F * Mod.");
    makeIn(Pin::Type::VALUE, "Drive", "0..1, Tanh-Saturation am Input mit Gain-Kompensation.");

    makeOut("LP_L", "Lowpass Links.");
    makeOut("LP_R", "Lowpass Rechts.");
    makeOut("HP_L", "Highpass Links.");
    makeOut("HP_R", "Highpass Rechts.");
    makeOut("BP_L", "Bandpass Links.");
    makeOut("BP_R", "Bandpass Rechts.");
    makeOut("N_L",  "Notch Links (= LP + HP).");
    makeOut("N_R",  "Notch Rechts (= LP + HP).");

    createProperties();
}

void StereoSVFilterModule::paint(juce::Graphics& g) {
    Module::paint(g);
}

void StereoSVFilterModule::recomputeCoefficients() {
    if (sampleRate <= 0.0) return;

    const double baseCutoff = juce::jlimit(20.0,
                                           sampleRate * 0.5 - 1.0,
                                           static_cast<double>(frequency) * static_cast<double>(mod));
    // Spread: L bekommt cutoff*(1-spread*0.1), R bekommt cutoff*(1+spread*0.1).
    // 10% maximaler Versatz reicht fuer hoerbar-stereo, ohne dass die Modi
    // zwischen L und R drastisch auseinanderlaufen.
    const double spreadFactor = 0.1 * static_cast<double>(spread);
    const double cutoffL = juce::jlimit(20.0, sampleRate * 0.5 - 1.0, baseCutoff * (1.0 - spreadFactor));
    const double cutoffR = juce::jlimit(20.0, sampleRate * 0.5 - 1.0, baseCutoff * (1.0 + spreadFactor));

    double Q = resonance;
    if (Q < 0.05) Q = 0.05;
    k = 1.0 / Q;

    auto coeffs = [&](double cutoff, double& a1, double& a2, double& a3) {
        const double g = std::tan(juce::MathConstants<double>::pi * cutoff / sampleRate);
        a1 = 1.0 / (1.0 + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    };
    coeffs(cutoffL, a1L, a2L, a3L);
    coeffs(cutoffR, a1R, a2R, a3R);
}

void StereoSVFilterModule::process() {
    if (pins.size() < 14 || sampleRate <= 0.0 || buffersize <= 0) return;

    // CV-Pins lesen, Coefficients ggf. neu rechnen.
    bool dirty = false;
    auto readVal = [&](int pinIdx, float& target, float deadband) -> bool {
        if (pins.at(pinIdx)->getConnections().size() != 1) return false;
        const float v = pins.at(pinIdx)->getConnections().at(0)->getValue();
        if (std::abs(v - target) > deadband) {
            target = v;
            return true;
        }
        return false;
    };
    if (readVal(2, frequency, 1.0e-3f)) dirty = true;
    if (readVal(3, resonance, 1.0e-4f)) dirty = true;
    if (readVal(4, mod,       1.0e-4f)) dirty = true;
    // Drive ist nur ein Pre-Stage-Multiplier, kein Coefficient — keine Coeff-
    // Berechnung noetig. Aber Wert holen.
    if (pins.at(5)->getConnections().size() == 1) {
        drive = juce::jlimit(0.0f, 1.0f, pins.at(5)->getConnections().at(0)->getValue());
    }
    if (dirty) recomputeCoefficients();

    juce::AudioSampleBuffer* inBufL = nullptr;
    juce::AudioSampleBuffer* inBufR = nullptr;
    if (pins.at(0)->getConnections().size() >= 1) inBufL = pins.at(0)->getConnections().at(0)->getAudioBuffer();
    if (pins.at(1)->getConnections().size() >= 1) inBufR = pins.at(1)->getConnections().at(0)->getAudioBuffer();

    juce::AudioSampleBuffer* outBuf[8];
    for (int i = 0; i < 8; ++i) outBuf[i] = pins.at(6 + i)->getAudioBuffer();
    for (int i = 0; i < 8; ++i)
        if (outBuf[i] == nullptr) return;

    // Kein Audio-In verbunden -> alle Outs leeren und raus.
    if (inBufL == nullptr && inBufR == nullptr) {
        for (int i = 0; i < 8; ++i) outBuf[i]->clear();
        return;
    }

    // Wenn nur ein Kanal verbunden ist, mono-fuettern (klassisch — wer
    // einen Mono-Saw an einen Stereo-Filter haengt, will ihn auf beiden
    // Seiten gleich klingen lassen).
    const float* inL = (inBufL != nullptr) ? inBufL->getReadPointer(0)
                     : (inBufR != nullptr ? inBufR->getReadPointer(0) : nullptr);
    const float* inR = (inBufR != nullptr) ? inBufR->getReadPointer(0)
                     : (inBufL != nullptr ? inBufL->getReadPointer(0) : nullptr);

    float* lpL = outBuf[0]->getWritePointer(0);
    float* lpR = outBuf[1]->getWritePointer(0);
    float* hpL = outBuf[2]->getWritePointer(0);
    float* hpR = outBuf[3]->getWritePointer(0);
    float* bpL = outBuf[4]->getWritePointer(0);
    float* bpR = outBuf[5]->getWritePointer(0);
    float* nL  = outBuf[6]->getWritePointer(0);
    float* nR  = outBuf[7]->getWritePointer(0);

    // Drive-Setup: tanh(x * gainIn) / gainOut. gainOut = tanh(gainIn) damit
    // Peaks bei +-1 nicht ueberkompensiert werden — bei drive=0 ist gainIn=1
    // und tanh(x)~=x fuer kleine x, also nahezu transparent.
    const double driveIn  = 1.0 + 4.0 * static_cast<double>(drive);
    const double driveOut = (drive > 0.0f) ? std::tanh(driveIn) : 1.0;

    auto svfStep = [](double v0, SVFStage& s, double a1, double a2, double a3, double k_,
                      double& outLP, double& outHP, double& outBP, double& outN) {
        const double v3 = v0 - s.ic2eq;
        const double v1 = a1 * s.ic1eq + a2 * v3;
        const double v2 = s.ic2eq + a2 * s.ic1eq + a3 * v3;
        s.ic1eq = 2.0 * v1 - s.ic1eq;
        s.ic2eq = 2.0 * v2 - s.ic2eq;
        outLP = v2;
        outBP = v1;
        outHP = v0 - k_ * v1 - v2;
        outN  = outLP + outHP;
    };

    for (int i = 0; i < buffersize; ++i) {
        // Drive
        double xL = static_cast<double>(inL[i]);
        double xR = static_cast<double>(inR[i]);
        if (drive > 0.0f) {
            xL = std::tanh(xL * driveIn) / driveOut;
            xR = std::tanh(xR * driveIn) / driveOut;
        }

        double lpLv, hpLv, bpLv, nLv;
        double lpRv, hpRv, bpRv, nRv;
        svfStep(xL, stageL[0], a1L, a2L, a3L, k, lpLv, hpLv, bpLv, nLv);
        svfStep(xR, stageR[0], a1R, a2R, a3R, k, lpRv, hpRv, bpRv, nRv);

        // 24dB-Kaskade: zweite SVF-Stage auf jedem Mode-Output. Klingt
        // steiler und etwas weniger resonant — pro Mode separat kaskadieren
        // damit z.B. der HP-Out auch wirklich 24dB Highpass ist.
        if (slope24) {
            double lp2, hp2, bp2, n2;
            svfStep(lpLv, stageL[1], a1L, a2L, a3L, k, lp2, hp2, bp2, n2); lpLv = lp2;
            svfStep(lpRv, stageR[1], a1R, a2R, a3R, k, lp2, hp2, bp2, n2); lpRv = lp2;
            // HP/BP/Notch durch dieselbe Stage zu schicken wuerde Coupling
            // erzeugen — fuer 24dB-Kaskade wirken alle 4 Modi auf den
            // gefilterten LP-Pfad nicht so wie erwartet. Also nur LP wird
            // kaskadiert; HP/BP/N bleiben 12dB. Dokumentationsdetail.
        }

        lpL[i] = static_cast<float>(lpLv);
        lpR[i] = static_cast<float>(lpRv);
        hpL[i] = static_cast<float>(hpLv);
        hpR[i] = static_cast<float>(hpRv);
        bpL[i] = static_cast<float>(bpLv);
        bpR[i] = static_cast<float>(bpRv);
        nL[i]  = static_cast<float>(nLv);
        nR[i]  = static_cast<float>(nRv);
    }
}

void StereoSVFilterModule::setSlope24(bool value) {
    if (slope24 == value) return;
    slope24 = value;
    // Kaskaden-Stage zuruecksetzen damit beim Umschalten kein DC-Versatz
    // aus altem Zustand stehenbleibt.
    stageL[1].reset();
    stageR[1].reset();
    if (slope24Value != nullptr)
        slope24Value->setValue(value);
}

void StereoSVFilterModule::setSpread(float value) {
    spread = juce::jlimit(0.0f, 1.0f, value);
    recomputeCoefficients();
    if (spreadValue != nullptr)
        spreadValue->setValue(spread);
}

void StereoSVFilterModule::createProperties() {
    slope24Value = new Value();
    slope24Listener = new Slope24Listener(*slope24Value, this);
    slope24Value->setValue(slope24);

    spreadValue = new Value();
    spreadListener = new SpreadListener(*spreadValue, this);
    spreadValue->setValue(spread);
}

juce::Array<PropertyComponent*>& StereoSVFilterModule::getProperties() {
    properties = juce::Array<PropertyComponent*>();
    slope24Prop = new BooleanPropertyComponent(*slope24Value, "Slope", "24 dB");
    spreadProp  = new SliderPropertyComponent(*spreadValue, "Spread", 0.0, 1.0, 0.01, 1.0, true);
    properties.add(slope24Prop);
    properties.add(spreadProp);
    return properties;
}
