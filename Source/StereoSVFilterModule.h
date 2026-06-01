/*
  ==============================================================================

    StereoSVFilterModule.h

    Stereo State-Variable Filter (TPT-Topologie nach Andrew Simper / Cytomic).
    Vier Modi gleichzeitig (LP, HP, BP, Notch) auf je eigenen Stereo-Output-
    Pins, plus optionale 24dB-Kaskade, Drive (Tanh-Saturation am Input mit
    Gain-Kompensation) und Stereo-Spread (Cutoff-Versatz L vs R).

    Filterzustand pro Kanal getrennt — keine Kopplung von L und R, damit
    Stereo-Material seinen Stereoeindruck behaelt. Coefficients werden nur
    bei Aenderung neu gerechnet (1e-3 Hysterese), pro Block einmal.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"

class StereoSVFilterModule : public Module
{
public:
    StereoSVFilterModule(double sampleRate, int buffersize);
    ~StereoSVFilterModule();

    void paint(juce::Graphics& g) override;
    void process() override;
    virtual void configurePins() override;

    virtual juce::String getCategory() override {
        return "Filter";
    }
    virtual juce::String getDescription() override {
        return "Stereo State-Variable Filter (TPT). InL/InR + F/Q/Mod/Drive. "
               "Outputs: LP_L/LP_R, HP_L/HP_R, BP_L/BP_R, N_L/N_R - alle "
               "Modi parallel. Properties: 24dB-Kaskade, Stereo-Spread "
               "(Cutoff-Versatz L/R fuer Stereo-Charakter).";
    }

    void setSlope24(bool value);
    bool isSlope24() const { return slope24; }

    void setSpread(float value);
    float getSpread() const { return spread; }

    virtual juce::Array<juce::PropertyComponent*>& getProperties() override;
    virtual void createProperties() override;

private:
    // Ein TPT-SVF-Stage. Stage-Coefficients sind ueber alle Stages gleich
    // (da gleicher Cutoff/Q), nur der Kanal-Zustand (ic1eq/ic2eq) ist getrennt.
    struct SVFStage {
        double ic1eq = 0.0;
        double ic2eq = 0.0;
        void reset() { ic1eq = 0.0; ic2eq = 0.0; }
    };

    void recomputeCoefficients();

    // -- DSP-Zustand
    SVFStage stageL[2];   // [0] = primary, [1] = optional 24dB-Kaskade
    SVFStage stageR[2];

    double a1L = 0.0, a2L = 0.0, a3L = 0.0;   // L-Kanal-Coefficients (mit Spread)
    double a1R = 0.0, a2R = 0.0, a3R = 0.0;   // R-Kanal
    double k = 1.0;                            // = 1/Q (kanalunabhaengig)

    float frequency = 1000.0f;
    float resonance = 1.0f;
    float mod = 1.0f;
    float drive = 0.0f;

    bool slope24 = false;
    float spread = 0.0f;     // 0..1; max ~10% Cutoff-Versatz L vs R

    // -- Properties
    struct Slope24Listener : juce::Value::Listener {
        Slope24Listener(juce::Value& v, StereoSVFilterModule* m) : m(m), value(v) { value.addListener(this); }
        ~Slope24Listener() {}
        void valueChanged(juce::Value& value) override {
            m->setSlope24(value.toString().getIntValue() > 0);
        }
        StereoSVFilterModule* m;
        juce::Value value;
    };
    struct SpreadListener : juce::Value::Listener {
        SpreadListener(juce::Value& v, StereoSVFilterModule* m) : m(m), value(v) { value.addListener(this); }
        ~SpreadListener() {}
        void valueChanged(juce::Value& value) override {
            m->setSpread((float) value.toString().getDoubleValue());
        }
        StereoSVFilterModule* m;
        juce::Value value;
    };

    juce::Value* slope24Value = nullptr;
    juce::PropertyComponent* slope24Prop = nullptr;
    Slope24Listener* slope24Listener = nullptr;

    juce::Value* spreadValue = nullptr;
    juce::PropertyComponent* spreadProp = nullptr;
    SpreadListener* spreadListener = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StereoSVFilterModule)
};
