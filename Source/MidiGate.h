#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"
#include <set>

//==============================================================================
// Ein polyphones Gate-Modul: sendet VALUE- & EVENT-Ausgänge, behält aber
// intern den Überblick über alle gerade gehaltenen Noten.
class MidiGate : public Module
{
public:
    MidiGate();
    ~MidiGate() override;

    // JUCE
    void paint(juce::Graphics&) override;
    void configurePins() override;
    void eventReceived(Event*) override;

    // Gate-API (von MidiProcessor aufgerufen)
    void gateOn(int velocity, int note);
    void gateOff(int note);

    // Properties
    juce::Array<juce::PropertyComponent*>& getProperties() override;
    void   createProperties() override;
    void   setChannel(int newChannel);
    int    getChannel() { return channel; }

    juce::String getCategory() override { return "MIDI"; }

private:
    // ----------------------------------------------------------------
    // GUI-Property-Listener
    struct ChannelListener : juce::Value::Listener
    {
        ChannelListener(juce::Value& v, MidiGate* g) : owner(g), value(v)
        {
            value.addListener(this);
        }

        void valueChanged(juce::Value&) override
        {
            owner->setChannel(value.getValue());
        }

        MidiGate* owner;
        juce::Value value;
    };

    // ----------------------------------------------------------------
    // Daten
    std::set<int> activeNotes;   // alle momentan gehaltenen Noten
    bool          gateActive{ false };

    int           channel{ 1 };
    juce::Value* channelValue{ nullptr };
    ChannelListener* channelListener{ nullptr };
    juce::PropertyComponent* channelProp{ nullptr };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiGate)
};
