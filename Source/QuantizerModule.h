/*
  ==============================================================================

    QuantizerModule.h

    Snap-to-Scale Quantizer fuer Pitch-CVs in Halbton-Einheiten (z.B. MIDI
    Note 0..127). Schickt zufallige oder linear gleitende Pitches durch eine
    musikalische Tonleiter, sodass aus „weisser Klangsuppe" wieder etwas
    Spielbares wird. Klassische Anwendung: Random/S&H + Quantizer + Sequencer.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"

class QuantizerModule : public Module
{
public:
    QuantizerModule();
    ~QuantizerModule();

    void paint(juce::Graphics& g) override;
    void process() override;
    virtual void configurePins() override;

    virtual juce::String getCategory() override {
        return "Math";
    }
    virtual juce::String getDescription() override {
        return "Quantizer. Rastet Pitch-CV (Halbtoeneinheiten, MIDI-Range) "
               "auf eine musikalische Skala. S = Skalen-Index "
               "(0=Chromatisch, 1=Dur, 2=Moll nat., 3=Moll harm., "
               "4=Pentatonisch Dur, 5=Pentatonisch Moll, 6=Blues, "
               "7=Dorisch, 8=Mixolydisch, 9=Ganzton). R = Root-Note "
               "(0=C, 1=C#, ... 11=H).";
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QuantizerModule)
};
