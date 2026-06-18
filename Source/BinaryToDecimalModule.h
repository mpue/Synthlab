/*
  ==============================================================================

    BinaryToDecimalModule.h

    Binaer -> Dezimal-Wandler mit konfigurierbarer Bit-Breite (1..16).
    Pins: Val (VALUE, Dezimal-Out), B0..Bn-1 (VALUE, je als Bit interpretiert:
    >= 0.5 zaehlt als 1). Out = sum(B_i * 2^i). Die Umkehrung des
    BinaryCounterModule.

    Pin-Anzahl waechst per Property-Slider (gleiches Pattern wie
    BinaryCounterModule — Pins koennen nur wachsen, nicht schrumpfen, sonst
    wuerden bestehende Verbindungen verloren gehen). Val liegt bewusst auf
    Index 0, damit sein Pin-Index beim Wachsen der Bits stabil bleibt.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"
#include <atomic>

class BinaryToDecimalModule : public Module
{
public:
    BinaryToDecimalModule();
    ~BinaryToDecimalModule();

    void paint(juce::Graphics& g) override;
    void process() override;
    virtual void configurePins() override;

    virtual juce::String getCategory() override {
        return "Logic";
    }
    virtual juce::String getDescription() override {
        return "Binaer -> Dezimal-Wandler. Pins: B0..Bn-1 (VALUE-Inputs, je als "
               "Bit interpretiert: >= 0.5 = 1), Val (Dezimalwert als VALUE-Out, "
               "sum(B_i * 2^i)). Bit-Breite per Property 1..16.";
    }

    void setNumBits(int newBits);

    virtual juce::Array<juce::PropertyComponent*>& getProperties() override;
    virtual void createProperties() override;

private:
    static constexpr int kMaxBits = 16;

    // Layout: [0]=Val (OUT), [1..]=B0..Bn-1 (IN).
    int bitInPinIndex(int bitIdx) const { return 1 + bitIdx; }

    struct BitsListener : juce::Value::Listener {
        BitsListener(juce::Value& v, BinaryToDecimalModule* m) : m(m), value(v) { value.addListener(this); }
        ~BitsListener() {}
        void valueChanged(juce::Value& value) override {
            m->setNumBits(value.toString().getIntValue());
        }
        BinaryToDecimalModule* m;
        juce::Value value;
    };

    juce::Value* bitsValue = nullptr;
    juce::PropertyComponent* bitsProp = nullptr;
    BitsListener* bitsListener = nullptr;

    int numBits = 4;
    std::atomic<int> lastValue { 0 };   // zuletzt berechneter Dezimalwert (fuer paint)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BinaryToDecimalModule)
};
