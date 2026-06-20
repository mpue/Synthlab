/*
  ==============================================================================

    StepSequencer8Module.h

    Klassischer 8-Step CV-Sequencer. Acht VALUE-Eingaenge S1..S8 liefern den
    Wert pro Step (z.B. von Knobs/Constants). Ein Clock-Event rueckt den
    aktiven Step um eins weiter und wrappt nach Step 8 zurueck auf Step 1.
    Der Gate-Eingang wirkt als zweiter Advance-Trigger (ODER-verknuepft mit
    Clock — jede steigende Flanke schaltet weiter). Reset springt auf Step 1.

    Ausgaenge: Value (VALUE) gibt den Wert des aktuell aktiven Steps live aus,
    Step (VALUE) den 0-basierten Step-Index 0..7 (z.B. fuer LED/Display).

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"
#include <atomic>

class StepSequencer8Module : public Module
{
public:
    StepSequencer8Module(double sampleRate, int buffersize);
    ~StepSequencer8Module();

    void paint(juce::Graphics& g) override;

    void process() override;
    virtual void configurePins() override;
    virtual void eventReceived(Event* e) override;

    virtual juce::String getCategory() override {
        return "MIDI";
    }
    virtual juce::String getDescription() override {
        return "8-Step CV-Sequencer. Pins: Clk (Event, rueckt den aktiven Step "
               "um 1 vor und wrappt), Rst (Event, springt auf Step 1), Gate "
               "(Event, zweiter Advance-Trigger ODER-verknuepft mit Clk), "
               "S1..S8 (VALUE, Wert pro Step). Out: Value (VALUE, Wert des "
               "aktiven Steps), Step (VALUE, Index 0..7), Gate (Event, feuert "
               "bei jedem Step ein Gate-On zum Triggern von Envelopes).";
    }

private:
    static constexpr int kNumSteps = 8;
    static constexpr int kGateNote = 60;     // feste Note fuer poly-ADSR-Voicing
    static constexpr int kGateVelocity = 127;

    float readStepValue(int step) const;     // VALUE am S(step+1)-Pin oder 0
    void  publishOutputs();                  // Value- und Step-Out aktualisieren
    void  sendGate(int velocity);            // GATE-Event an alle Gate-Out-Verbindungen

    Pin* clockIn = nullptr;
    Pin* resetIn = nullptr;
    Pin* gateIn  = nullptr;
    Pin* stepIns[kNumSteps] = { nullptr };
    Pin* valueOut = nullptr;
    Pin* stepOut  = nullptr;
    Pin* gateOut  = nullptr;

    std::atomic<int> currentStep { 0 };    // Audio-Thread liest, Event-Thread schreibt
    // "Scharf" nach CLOCK_START/Reset: der naechste Clock-Tick spielt Step 1,
    // OHNE vorher zu advancen — so startet der Sequencer taktbuendig auf S1.
    std::atomic<bool> armed { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StepSequencer8Module)
};
