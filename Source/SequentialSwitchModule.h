/*
  ==============================================================================

    SequentialSwitchModule.h

    1-zu-N Audio-Router. Ein Audio-Eingang wird auf eine konfigurierbare
    Anzahl Audio-Ausgaenge verteilt — pro Clock-Event rueckt der aktive
    Ausgang um eins weiter und wrappt am Ende, alle anderen Ausgaenge
    bleiben stumm. Klassischer Eurorack-Sequential-Switch.

    Pin-Anzahl ist nach Konstruktion vergroessern via Property-Slider
    (gleiches Pattern wie MixerModule — Pins koennen nur wachsen, nicht
    schrumpfen, weil das bestehende Verbindungen kosten wuerde).

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"
#include <atomic>

class SequentialSwitchModule : public Module
{
public:
    SequentialSwitchModule(double sampleRate, int buffersize);
    ~SequentialSwitchModule();

    void paint(juce::Graphics& g) override;

    void process() override;
    virtual void configurePins() override;
    virtual void eventReceived(Event* e) override;

    virtual juce::String getCategory() override {
        return "Audio";
    }
    virtual juce::String getDescription() override {
        return "1-zu-N Audio-Sequential-Switch. Pins: In (Audio), Clk (Event, "
               "rueckt den aktiven Out um 1 vor), Rst (Event, springt auf Out1). "
               "Out1..OutN bekommen das Eingangssignal sequentiell. Step (VALUE) "
               "gibt den aktuellen Index aus. Anzahl Outs per Property 2..16.";
    }

    void setNumOutputs(int newCount);

    virtual juce::Array<juce::PropertyComponent*>& getProperties() override;
    virtual void createProperties() override;

private:
    static constexpr int kMaxOutputs = 16;

    void writeActiveOutput(const float* in, int sampleCount);
    void clearOutputs(int sampleCount);
    int  audioOutPinIndex(int outIdx) const;     // wo im pins[]-Array Out_i liegt
    Pin* getStepOutPin() const;                   // VALUE-OUT pin

    // -- Property-Listener
    struct OutputsListener : juce::Value::Listener {
        OutputsListener(juce::Value& v, SequentialSwitchModule* m) : m(m), value(v) { value.addListener(this); }
        ~OutputsListener() {}
        void valueChanged(juce::Value& value) override {
            m->setNumOutputs(value.toString().getIntValue());
        }
        SequentialSwitchModule* m;
        juce::Value value;
    };

    juce::Value* outputsValue = nullptr;
    juce::PropertyComponent* outputsProp = nullptr;
    OutputsListener* outputsListener = nullptr;

    int numOutputs = 4;
    std::atomic<int> currentStep { 0 };          // Audio-Thread liest, Event-Thread schreibt

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequentialSwitchModule)
};
