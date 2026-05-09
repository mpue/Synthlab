/*
  ==============================================================================

    AttenuverterModule.h

    out = in * scale + offset

    Klassisches Eurorack-Util: skaliert ein eingehendes CV-Signal und schiebt
    es. Scale ist signed (negativ = invertierend → "Atten-uVerter"), Offset
    additiv. Reicht in 99% der Faelle, um Hüllkurven/LFOs/Sequencer-Out auf
    den richtigen Wertebereich für irgendeinen Mod-Input zu bringen.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"

class AttenuverterModule : public Module
{
public:
    AttenuverterModule();
    ~AttenuverterModule();

    void paint(juce::Graphics& g) override;
    void process() override;
    virtual void configurePins() override;

    virtual juce::String getCategory() override {
        return "Math";
    }
    virtual juce::String getDescription() override {
        return "Attenuverter: out = in * scale + offset. "
               "Scale ist signed (negative Werte invertieren), Offset additiv. "
               "Standard-CV-Util zum Anpassen von Modulationsamplitude/Bias.";
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AttenuverterModule)
};
