/*
 ==============================================================================
 
 MidiClock.cpp
 Created: 4 Apr 2018 3:48:23pm
 Author:  Matthias Pueski
 
 ==============================================================================
 */

#include "../JuceLibraryCode/JuceHeader.h"
#include "MidiClock.h"
#include "Connection.h"
#include "MidiOut.h"

using juce::Justification;
using juce::ImageCache;
using juce::Value;
using juce::PropertyComponent;
using juce::SliderPropertyComponent;
using juce::Array;

//==============================================================================
MidiClock::MidiClock()
{
    setSize(120,140);
    setName("Midi Clock");
    
    editable = false;
    prefab = true;
    
    createProperties();
}

MidiClock::~MidiClock()
{
    // Timer zuerst stoppen, sonst kann er nach dem Loeschen der Member-
    // Values noch feuern und in den toten Listener-Callback laufen (UAF).
    stopTimer();
    delete channelValue;
    delete channelListener;
    delete tempoValue;
    delete tempoListener;
    delete internalValue;
    delete internalListener;
}

void MidiClock::configurePins() {
    
    Pin* p1 = new Pin(Pin::Type::VALUE);
    p1->direction = Pin::Direction::OUT;
    p1->listeners.push_back(this);
    p1->setName("G");
    p1->setDescription("Gate value out. Contains velocity and note value as well.");
    addPin(Pin::Direction::OUT,p1);
    
    Pin* p2 = new Pin(Pin::Type::EVENT);
    p2->direction = Pin::Direction::OUT;
    p2->listeners.push_back(this);
    p2->setName("E");
    p2->setDescription("Event out. Contains the velocity and note values of all 128 notes as well.");
    addPin(Pin::Direction::OUT,p2);
}

void MidiClock::paint(juce::Graphics &g) {
    Module::paint(g);
    g.drawImageAt(ImageCache::getFromMemory(BinaryData::clock_png, BinaryData::clock_pngSize),25,40);
}

void MidiClock::hiResTimerCallback() {

    // Phase-locked: feuere so viele Ticks wie noetig, bis nextTickMs in
    // der Zukunft liegt. So wird kumulativer Drift vermieden (jeder Tick
    // ist absolut geplant, nicht relativ zum letzten Aufruf), und bei
    // einem Thread-Hiccup holen wir die verlorenen Ticks zurueck statt
    // sie wegzulassen.
    const double nowMs = juce::Time::getMillisecondCounterHiRes();

    while (running && nowMs >= nextTickMs) {
        Event* e = new Event("clock", Event::Type::CLOCK);
        MidiMessage m = MidiMessage::midiClock();
        e->setMessage(m);
        e->setValue((int) tempo);
        for (int i = 0; i < pins.at(1)->getConnections().size(); i++) {
            pins.at(1)->getConnections().at(i)->sendEvent(new Event(e));
        }
        delete e;

        nextTickMs += periodMs;
    }
}

void MidiClock::clock(juce::MidiMessage message) {
    Event* e = new Event("clock",Event::Type::CLOCK);
    e->setMessage(message);
    // Selbes Problem wie in timerCallback: die eintreffende Clock-Message
    // traegt kein Tempo, also nehmen wir unseren konfigurierten BPM-Wert.
    e->setValue((int) tempo);
    for (int i = 0; i < pins.at(1)->getConnections().size();i++) {
        pins.at(1)->getConnections().at(i)->sendEvent(new Event(e));
    }

    delete e;
}

void MidiClock::midiStart() {
    Event* e = new Event("clock",Event::Type::CLOCK_START);
    
    for (int i = 0; i < pins.at(1)->getConnections().size();i++) {
        pins.at(1)->getConnections().at(i)->sendEvent(new Event(e));
    }
    
    delete e;
}

void MidiClock::midiStop() {
    Event* e = new Event("clock",Event::Type::CLOCK_STOP);
    
    for (int i = 0; i < pins.at(1)->getConnections().size();i++) {
        pins.at(1)->getConnections().at(i)->sendEvent(new Event(e));
    }
    
    delete e;
}

void MidiClock::createProperties() {
    channelValue = new Value();
    channelListener = new ChannelListener(*channelValue, this);
    internalValue = new Value();
    internalListener = new InternalClockListener(*internalValue,this);
    tempoValue = new Value();
    tempoValue->setValue(tempo);
    tempoListener = new TempoListener(*tempoValue,this);
}

juce::Array<PropertyComponent*>& MidiClock::getProperties() {
    
    properties = juce::Array<PropertyComponent*>();
    
    channelProp = new SliderPropertyComponent(*channelValue,"channel",1,16,1,1,true);
    tempoProp = new SliderPropertyComponent(*tempoValue,"Tempo",1,240,0.1,1.0,true);
    internalProp = new BooleanPropertyComponent(*internalValue,"Internal","Internal");
    
    properties.add(channelProp);
    properties.add(tempoProp);
    properties.add(internalProp);
    
    return properties;
}

int MidiClock::getChannel() {
    return channel;
}

void MidiClock::setChannel(int channel) {
    if (channel < 1) {
        channel = 1;
    }
    this->channel = channel;
    this->channelValue->setValue(channel);
}

void MidiClock::setInternal(bool isInternal) {
    if (isInternal) {

        // MIDI-Clock-Spec: 24 PPQN. periodMs ist die exakte (fraktionale)
        // Tick-Periode; nextTickMs wird beim Start auf "jetzt" geseedet,
        // damit der erste Tick sofort feuert.
        periodMs   = 60000.0 / (24.0 * tempo);
        nextTickMs = juce::Time::getMillisecondCounterHiRes();

        if (running) {
            stopTimer();
        }
        // Poll-Periode 1 ms — feiner als das eigentliche Tick-Intervall
        // (~20.83 ms bei 120 BPM). Die hiResTimerCallback feuert Ticks
        // basierend auf nextTickMs, nicht auf der Poll-Periode, also
        // begrenzt 1 ms hier nur den Jitter auf <=1 ms.
        startTimer(1);
        running = true;
    }
    else {
        if (running) {
            stopTimer();
            // Vorher wurde running nicht zurueckgesetzt -> der naechste
            // setInternal(true) lief durch den else-Zweig (stopTimer +
            // startTimer) statt einfach startTimer.
            running = false;
        }
    }
    this->internalClock = isInternal;
    this->internalValue->setValue(isInternal);
}

bool MidiClock::isInternal() {
    return internalClock;
}

void MidiClock::setTempo(float tempo) {
    this->tempo = tempo;
    this->tempoValue->setValue(tempo);
    // Vorher: setInternal(tempo) — der float wurde implizit auf bool gecastet
    // und damit der Internal-Mode bei jeder Tempo-Aenderung erzwungen.
    // Jetzt nur die Tick-Periode neu berechnen, falls der interne Clock
    // laeuft. nextTickMs nicht resetten, sonst gibt's an der Tempo-Aenderung
    // einen Phasensprung — der naechste Tick rutscht einfach an die neue
    // Periode.
    if (running) {
        periodMs = 60000.0 / (24.0 * tempo);
    }
}

float MidiClock::getTempo() {
    return tempo;
}


