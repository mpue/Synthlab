/*
  ==============================================================================

    SampleAndHoldModule.h

    Sample & Hold. Auf jedes eingehende Gate-/Trigger-Event nimmt das Modul
    den aktuellen In-Wert und haelt ihn bis zum naechsten Trigger am Out-Pin.
    Klassische Anwendung: Noise auf In, Clock auf T → "Random-Melodie", die
    pro Tick neu wuerfelt aber zwischen Ticks stabil bleibt (statt staendig
    zu zappeln). Auch sinnvoll als Debounce fuer rauschende CVs.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"

class SampleAndHoldModule : public Module
{
public:
    SampleAndHoldModule();
    ~SampleAndHoldModule();

    void paint(juce::Graphics& g) override;
    void process() override;
    virtual void eventReceived(Event* e) override;
    virtual void configurePins() override;

    virtual juce::String getCategory() override {
        return "Math";
    }
    virtual juce::String getDescription() override {
        return "Sample & Hold. Sampelt den In-Wert auf jedes Gate-Event am "
               "Trigger-Pin und haelt ihn am Out-Pin. Klassisch: Noise + "
               "Clock fuer Random-Melodien.";
    }

private:
    float held = 0.0f;
    Pin* triggerPin = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleAndHoldModule)
};
