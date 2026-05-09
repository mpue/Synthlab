/*
  ==============================================================================

    SampleAndHoldModule.cpp

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "SampleAndHoldModule.h"

SampleAndHoldModule::SampleAndHoldModule()
{
    setSize(120, 140);
    setName("S&H");

    editable = false;
    prefab = true;
}

SampleAndHoldModule::~SampleAndHoldModule()
{
}

void SampleAndHoldModule::configurePins() {
    // pins[0] = In  (VALUE) — Signal zum Sampeln
    // pins[1] = Trig (EVENT) — auf jedes Gate wird gesampelt
    // pins[2] = Out (VALUE)  — gehaltener Wert
    Pin* in = new Pin(Pin::Type::VALUE);
    in->direction = Pin::Direction::IN;
    in->listeners.push_back(this);
    in->setName("In");
    in->setDescription("Eingang. Wert wird auf Trigger gesampelt.");

    triggerPin = new Pin(Pin::Type::EVENT);
    triggerPin->direction = Pin::Direction::IN;
    triggerPin->listeners.push_back(this);
    triggerPin->setName("T");
    triggerPin->setDescription("Trigger-Event. Jedes Gate-On laesst das Modul den aktuellen In-Wert kapieren.");

    Pin* out = new Pin(Pin::Type::VALUE);
    out->direction = Pin::Direction::OUT;
    out->listeners.push_back(this);
    out->setName("Out");
    out->setDescription("Zuletzt gesampelter Wert (gehalten).");

    addPin(Pin::Direction::IN, in);
    addPin(Pin::Direction::IN, triggerPin);
    addPin(Pin::Direction::OUT, out);
}

void SampleAndHoldModule::paint(juce::Graphics& g) {
    Module::paint(g);
}

void SampleAndHoldModule::process() {
    if (pins.size() < 3) return;
    // Out gibt einfach den gehaltenen Wert pro Block weiter — der eigentliche
    // Sample-Vorgang passiert event-getrieben in eventReceived().
    pins.at(2)->setValue(held);
}

void SampleAndHoldModule::eventReceived(Event* e) {
    if (e == nullptr) return;
    if (e->getType() != Event::Type::GATE) return;
    // Nur auf Gate-On sampeln (value > 0); Gate-Off ignorieren.
    if (e->getValue() <= 0) return;
    if (pins.size() < 1) return;

    if (pins.at(0)->getConnections().size() == 1) {
        held = pins.at(0)->getConnections().at(0)->getValue();
    }
    // Sofort auch nach aussen propagieren, sonst zieht der Out-Pin erst
    // beim naechsten process()-Aufruf nach.
    if (pins.size() >= 3)
        pins.at(2)->setValue(held);
}
