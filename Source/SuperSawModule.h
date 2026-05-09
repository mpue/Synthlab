/*
  ==============================================================================

    SuperSawModule.h

    7-voice detuned-saw stack im Stil von JP-8000 / Roland Supersaw.
    Pro gespielter Note 7 interne Sawtooth-Oszillatoren mit verteiltem
    Detune (Cent-Spread) und stereo aufgefaecherter Panning. Polyphon
    (per-Note-Voice-Slot 0..127, wie SawtoothModule) oder mono.

    Kontroll-Pins:
      P   - Pitch in Halbtoenen (Transpose).
      F   - Fine in Hz (additiv auf Frequenz).
      A   - Gate-/Volume-Pin (z.B. von MIDI-Gate oder ADSR). Wenn die
            Source polyphon ist (dataEnabled[] gesetzt), spielt jede
            aktive Note 7 Stimmen gleichzeitig.
      D   - Detune-Amount 0..1 -> 0..50 Cent Spread. Klassischer
            Supersaw-Sound liegt bei ~0.3..0.6.
      M   - Mix 0..1. Balance zwischen Center-Voice und Side-Voices.
            0 = nur Center (= einfacher Saw), 1 = nur Sides.
      W   - Stereo-Width 0..1. 0 = Mono (alle Voices in der Mitte),
            1 = volle Auffaecherung links/rechts.

    Audio-Outs L, R sind separate AUDIO-Pins (selbe Konvention wie im
    Stereo-Delay), Mono-Quellen koennen den L-Pin doppelt abgreifen.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"
#include "AudioEngine/Sawtooth.h"

class SuperSawModule : public Module
{
public:
    SuperSawModule(double sampleRate, int buffersize);
    ~SuperSawModule();

    void paint(juce::Graphics& g) override;

    void process() override;
    virtual void configurePins() override;

    virtual juce::String getCategory() override {
        return "Sound sources";
    }
    virtual juce::String getDescription() override {
        return "7-voice Supersaw. Pins: P=Pitch (Halbtoene), F=Fine (Hz), "
               "A=Gate (von MIDI-Gate/ADSR, polyphon ueber dataEnabled[]), "
               "D=Detune 0..1 (= 0..50 Cent Spread), M=Mix 0..1 "
               "(Center vs Sides), W=Width 0..1 (Stereo-Spread). "
               "Outputs L, R audio plus V (Peak-Monitor).";
    }

    bool isMono();
    void setMono(bool value);

    virtual juce::Array<juce::PropertyComponent*>& getProperties() override;
    virtual void createProperties() override;

private:
    static constexpr int kNumVoices = 7;       // klassische Supersaw-Stimmenzahl

    void recomputeVoiceMix();                  // mix/detune/width -> Cache neu
    void updateVoiceFrequencies(int slot, double baseFreq);

    // -- Properties (Mono-Toggle wie SawtoothModule)
    struct IsMonoListener : juce::Value::Listener {
        IsMonoListener(juce::Value& v, SuperSawModule* mod) : mod(mod), value(v) { value.addListener(this); }
        ~IsMonoListener() {}
        void valueChanged(juce::Value& value) override {
            mod->setMono(value.toString().getIntValue() > 0);
        }
        SuperSawModule* mod;
        juce::Value value;
    };

    juce::Value* isMonoValue = nullptr;
    juce::PropertyComponent* isMonoProp = nullptr;
    IsMonoListener* isMonoListener = nullptr;

    // -- DSP-Zustand
    juce::AudioSampleBuffer* outL = nullptr;
    juce::AudioSampleBuffer* outR = nullptr;

    int   pitch = 0;
    float fine = 0.0f;
    float detuneAmount = 0.5f;     // 0..1
    float mixAmount = 0.5f;        // 0..1
    float widthAmount = 1.0f;      // 0..1

    // Cached pro-Voice-Werte (werden in recomputeVoiceMix() neu gerechnet)
    float voiceCentOffset[kNumVoices] = {0};   // Cent-Versatz vom Center
    float voiceGain[kNumVoices] = {0};         // Gain inkl. Mix-Curve
    float voicePanL[kNumVoices] = {0};         // Equal-Power L
    float voicePanR[kNumVoices] = {0};         // Equal-Power R

    // Mono-Stimme: 7 saws, ein Slot. Polyphon: 128 Slots a 7 saws.
    Sawtooth* monoVoice[kNumVoices] = {nullptr};
    Sawtooth* polyVoice[128][kNumVoices] = {{nullptr}};
    bool      voiceActive[128] = {false};

    bool mono = false;
    Pin* gatePin = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SuperSawModule)
};
