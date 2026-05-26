/*
  ==============================================================================

    TFlipFlopModule.h

    T-Flip-Flop: toggelt seinen Zustand bei jedem Gate-On am T-Input.
    Pins: T (Event, In), Rst (Event, In), Q (Value, Out), /Q (Value, Out).

    Halbierter Takt: an T angeschlossen, liefert Q einen Clock-Out mit halber
    Frequenz — typischer Use-Case fuer den ersten Bit-Out eines Zaehlers ohne
    den ganzen BinaryCounterModule-Overhead.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"
#include <atomic>

class TFlipFlopModule : public Module
{
public:
    TFlipFlopModule();
    ~TFlipFlopModule();

    void paint(juce::Graphics& g) override;
    void process() override;
    virtual void eventReceived(Event* e) override;
    virtual void configurePins() override;

    virtual juce::String getCategory() override {
        return "Logic";
    }
    virtual juce::String getDescription() override {
        return "T-Flip-Flop. Toggelt Q bei jedem Gate-On am T-Input. "
               "Rst setzt Q=0. /Q ist das Komplement. Mit T = Clock liefert "
               "Q einen halbierten Takt.";
    }

private:
    std::atomic<bool> state { false };
    // Rising-Edge-Detection auf dem Value-T-Eingang: process() merkt sich den
    // letzten Sample-Wert; ein Uebergang von <0.5 nach >=0.5 zaehlt als Toggle.
    // Schwellwert in der Mitte, damit verrauschte 0/1-Signale (z.B. ungefilterte
    // PWM) nicht mehrfach pro logischer Flanke triggern.
    float lastTValue = 0.0f;
    float lastRstValue = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TFlipFlopModule)
};
