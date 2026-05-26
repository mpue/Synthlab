/*
  ==============================================================================

    BinaryCounterModule.h

    Binaerer Zaehler mit konfigurierbarer Bit-Breite (1..16).
    Pins: Clk (Event, +1 pro Gate-On), Rst (Event, =0), Val (VALUE, Dezimalwert),
    B0..Bn-1 (VALUE, je 0 oder 1). Wrappt bei 2^Bits.

    Pin-Anzahl waechst per Property-Slider (gleiches Pattern wie
    SequentialSwitchModule — Pins koennen nur wachsen, nicht schrumpfen, sonst
    wuerden bestehende Verbindungen verloren gehen).

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"
#include <atomic>

class BinaryCounterModule : public Module
{
public:
    BinaryCounterModule();
    ~BinaryCounterModule();

    void paint(juce::Graphics& g) override;
    void process() override;
    virtual void eventReceived(Event* e) override;
    virtual void configurePins() override;

    virtual juce::String getCategory() override {
        return "Logic";
    }
    virtual juce::String getDescription() override {
        return "Binaerer Zaehler. Pins: Clk (Event, +1 pro Gate-On), "
               "Rst (Event, setzt auf 0), Val (Dezimalwert), "
               "B0..Bn-1 (Bit-Outputs, je 0 oder 1). "
               "Bit-Breite per Property 1..16. Wrappt bei 2^Bits.";
    }

    void setNumBits(int newBits);

    virtual juce::Array<juce::PropertyComponent*>& getProperties() override;
    virtual void createProperties() override;

private:
    static constexpr int kMaxBits = 16;

    int bitOutPinIndex(int bitIdx) const;

    struct BitsListener : juce::Value::Listener {
        BitsListener(juce::Value& v, BinaryCounterModule* m) : m(m), value(v) { value.addListener(this); }
        ~BitsListener() {}
        void valueChanged(juce::Value& value) override {
            m->setNumBits(value.toString().getIntValue());
        }
        BinaryCounterModule* m;
        juce::Value value;
    };

    juce::Value* bitsValue = nullptr;
    juce::PropertyComponent* bitsProp = nullptr;
    BitsListener* bitsListener = nullptr;

    int numBits = 4;
    std::atomic<int> counter { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BinaryCounterModule)
};
