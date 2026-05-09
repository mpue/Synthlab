/*
  ==============================================================================

    ClockDividerModule.h

    Zaehlt Clock-Events am Eingang und gibt nur jedes N-te am Ausgang
    weiter. Klassischer Sync-Util fuer modulare Setups: ein 16tel-Clock am
    In, Divider auf 4, raus kommen Viertel — fuer einen langsameren
    Sequencer / LFO-Sync / Drum-Pattern. Reset-Pin stellt den Zaehler
    wieder auf 0 (= naechstes Tick gibt sofort aus).

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"

class ClockDividerModule : public Module
{
public:
    ClockDividerModule();
    ~ClockDividerModule();

    void paint(juce::Graphics& g) override;
    void process() override;
    virtual void eventReceived(Event* e) override;
    virtual void configurePins() override;

    virtual juce::String getCategory() override {
        return "MIDI";
    }
    virtual juce::String getDescription() override {
        return "Clock Divider. Zaehlt eingehende Clock-/Gate-Events und "
               "gibt jedes N-te am Out weiter (N = Divider-Pin, default 4). "
               "Reset-Pin setzt den Zaehler zurueck. Negative oder 0 als "
               "Divider werden auf 1 geklemmt (= Pass-Through).";
    }

private:
    Pin* clockIn = nullptr;
    Pin* divPin = nullptr;
    Pin* clockOut = nullptr;

    int counter = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClockDividerModule)
};
