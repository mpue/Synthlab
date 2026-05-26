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
    // pins[1] = Trig (EVENT) — auf jedes Gate / Clock wird gesampelt
    // pins[2] = Out (VALUE)  — gehaltener Wert
    Pin* in = new Pin(Pin::Type::VALUE);
    in->direction = Pin::Direction::IN;
    in->setName("In");
    in->setDescription("Eingang. Wert wird auf Trigger gesampelt.");

    triggerPin = new Pin(Pin::Type::EVENT);
    triggerPin->direction = Pin::Direction::IN;
    // Nur am EVENT-Pin als Listener anhaengen — In/Out sind VALUE-Pins und
    // dispatchen ohnehin keine Events. Der ueberfluessige listener-push auf
    // In/Out hat keinen Schaden, aber es verschleiert die Rolle.
    triggerPin->listeners.push_back(this);
    triggerPin->setName("T");
    triggerPin->setDescription("Trigger-Event. Jedes Gate-On / CLOCK-Event laesst das Modul den aktuellen In-Wert kapieren.");

    Pin* out = new Pin(Pin::Type::VALUE);
    out->direction = Pin::Direction::OUT;
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

    // Akzeptier-Kriterium analog zu BinaryCounter/T-Flip-Flop: CLOCK
    // (MidiClock-Pulse) ODER Gate-On (value > 0). Frueher war nur Gate-On
    // erlaubt — eine direkte Clock-Quelle hat dadurch nichts ausgeloest und
    // der Out blieb auf seinem Startwert kleben.
    const auto type = e->getType();
    const bool isTrigger =
        type == Event::Type::CLOCK
        || (type == Event::Type::GATE && e->getValue() > 0);
    if (!isTrigger) return;

    if (pins.size() < 1) return;

    if (pins.at(0)->getConnections().size() == 1
        && pins.at(0)->getConnections().at(0) != nullptr) {
        held = pins.at(0)->getConnections().at(0)->getValue();
    }
    // Sofort auch nach aussen propagieren, sonst zieht der Out-Pin erst
    // beim naechsten process()-Aufruf nach.
    if (pins.size() >= 3)
        pins.at(2)->setValue(held);
}
