/*
  ==============================================================================

    SVFilterModule.h

    State-Variable Filter (TPT-Topology nach Andrew Simper / Cytomic). Multi-
    Mode: LP, HP, BP und Notch werden gleichzeitig berechnet und stehen an
    eigenen Output-Pins zur Verfuegung. Stabil ueber den ganzen Cutoff- und
    Resonanz-Bereich (im Gegensatz zum klassischen IIR-Lowpass, der bei sehr
    kleinen Cutoffs in den Anschlag laeuft).

    Mod-Pin: multiplikativ wie beim LPFilter (cutoff = frequency * mod).
    Floor bei 20 Hz gegen IIR-Rauchwolken auch wenn das hier deutlich
    weniger relevant ist als beim alten Filter — TPT ist auch bei sehr
    kleinem g rechnerisch ok, aber unhoerbar.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"

class SVFilterModule : public Module
{
public:
    SVFilterModule(double sampleRate, int buffersize);
    ~SVFilterModule();

    void paint(juce::Graphics& g) override;
    void process() override;
    virtual void configurePins() override;

    virtual juce::String getCategory() override {
        return "Filter";
    }
    virtual juce::String getDescription() override {
        return "State-Variable Filter (TPT). Pins F (Cutoff Hz), Q (Resonanz), "
               "Mod (multiplikativer Cutoff-Modulator), In (audio). "
               "Outputs: LP, HP, BP, Notch — alle parallel berechnet. "
               "Stabil ueber den gesamten Cutoff/Q-Bereich.";
    }

private:
    void recomputeCoefficients();

    float frequency = 1000.0f;
    float resonance = 1.0f;
    float mod = 1.0f;

    // TPT-Coefficients
    double a1 = 0.0, a2 = 0.0, a3 = 0.0;
    double k = 1.0;     // = 1/Q

    // Filterzustand (mono — Input wird nur aus Channel 0 gelesen, wie beim LP/HP)
    double ic1eq = 0.0;
    double ic2eq = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SVFilterModule)
};
