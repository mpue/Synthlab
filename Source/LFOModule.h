/*
  ==============================================================================

    LFOModule.h

    Low-Frequency Oscillator. 5 Wellenformen + Sample-and-Hold, bipolar
    (-1..1) und unipolar (0..1) Outputs, optionaler Hard-Sync via Trigger-Pin.

    Eigene leichte Implementierung — bewusst nicht ueber die bestehenden
    Oszi-Module, weil die fuer Audio-Rate ausgelegt sind. Eine LFO will
    genau das Gegenteil: stabile Sub-Hz-Frequenzen, definierte Shapes,
    günstige CPU-Last.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"
#include <random>

class LFOModule : public Module
{
public:
    LFOModule(double sampleRate, int buffersize);
    ~LFOModule();

    void paint(juce::Graphics& g) override;
    void process() override;
    virtual void eventReceived(Event* e) override;
    virtual void configurePins() override;

    virtual juce::String getCategory() override {
        return "Sound sources";
    }
    virtual juce::String getDescription() override {
        return "Low-Frequency Oscillator. Pin S waehlt die Wellenform: "
               "0=Sine, 1=Triangle, 2=Saw up, 3=Saw down, 4=Square, "
               "5=Sample-and-Hold. R = Rate in Hz (default 1.0). "
               "T = Hard-Sync Trigger (Phase auf 0). Outputs sind "
               "Bi (bipolar, -1..1) und Uni (unipolar, 0..1).";
    }

private:
    enum Shape {
        SINE = 0,
        TRIANGLE = 1,
        SAW_UP = 2,
        SAW_DOWN = 3,
        SQUARE = 4,
        SAMPLE_HOLD = 5
    };

    double phase = 0.0;          // 0..1
    float shFrozen = 0.0f;       // gehaltener Wert fuer S&H-Shape
    bool shFresh = false;        // sicherstellen dass beim ersten Tick gesampelt wird
    std::mt19937 rng { 0xC0FFEE };
    std::uniform_real_distribution<float> uni { -1.0f, 1.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LFOModule)
};
