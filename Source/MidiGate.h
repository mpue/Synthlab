#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"
#include <set>

class Track;  // forward decl — gate only stores a pointer for identity routing

//==============================================================================
// Ein polyphones Gate-Modul: sendet VALUE- & EVENT-Ausg�nge, beh�lt aber
// intern den �berblick �ber alle gerade gehaltenen Noten.
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

    // Identity-based live routing: when this gate belongs to an instrument/MIDI
    // track, the MidiProcessor fires it by Track identity instead of MIDI
    // channel, so two tracks can share a channel without overlapping. A null
    // track means "modular gate" -> classic channel matching. trackResolved
    // distinguishes "known to have no track" from "not yet looked up" so the
    // processor only needs to walk the connection graph once per gate.
    void   setTrack(Track* t) { track = t; trackResolved = true; }
    Track* getTrack() const { return track; }
    bool   isTrackResolved() const { return trackResolved; }
    void   invalidateTrack() { track = nullptr; trackResolved = false; }

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

    Track*        track{ nullptr };       // owning track for identity routing
    bool          trackResolved{ false }; // false until the graph has been walked

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiGate)
};
