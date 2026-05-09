/*
  ==============================================================================

    SlewLimiterModule.h

    Linearer Slew-Limiter / Glide / Portamento. Begrenzt die maximale
    Aenderungsrate des Eingangswertes; rise- und fall-time getrennt
    einstellbar (in Sekunden pro Einheit). Damit baut man:
      - klassisches Portamento auf einer Pitch-CV
      - Smoother fuer Sequencer-/S&H-Output
      - Soft-Gate aus harten Trigger-Pegeln.

    Slew passiert auf Block-Rate (das reicht fuer VALUE-Pins, die ohnehin
    nur einmal pro Block aktualisiert werden — Granularitaet ist
    bufferSize/sampleRate, typisch 5-20 ms).

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"

class SlewLimiterModule : public Module
{
public:
    SlewLimiterModule(double sampleRate, int buffersize);
    ~SlewLimiterModule();

    void paint(juce::Graphics& g) override;
    void process() override;
    virtual void configurePins() override;

    virtual juce::String getCategory() override {
        return "Math";
    }
    virtual juce::String getDescription() override {
        return "Slew Limiter / Portamento. Begrenzt die Aenderungsrate des "
               "Eingangs auf 1 Einheit pro Rise- bzw. Fall-Zeit (Sekunden). "
               "Klassischer Glide-Effekt auf einer Pitch-CV.";
    }

private:
    float currentOut = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SlewLimiterModule)
};
