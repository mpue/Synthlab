/*
  ==============================================================================

    ClockDividerModule.cpp

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "ClockDividerModule.h"

ClockDividerModule::ClockDividerModule()
{
    setSize(120, 140);
    setName("Clock Div");

    editable = false;
    prefab = true;
}

ClockDividerModule::~ClockDividerModule()
{
}

void ClockDividerModule::configurePins() {
    // pins[0] = Clk (EVENT) - clock in
    // pins[1] = N   (VALUE) - divider, default 4
    // pins[2] = Out (EVENT) - divided clock out
    //
    // (Reset-Pin koennte man dazubauen, braucht aber eine
    // Source-Pin-Disambiguierung im Event-Modell, die's bisher in
    // Synthlab nicht gibt — alle event-listenenden Pins eines Moduls
    // teilen sich einen eventReceived-Callback. Bis das gemacht ist,
    // einfach Clock-Eingang abklemmen oder anders Pause machen, wenn
    // man den Zaehler auf 0 will.)
    clockIn = new Pin(Pin::Type::EVENT);
    clockIn->direction = Pin::Direction::IN;
    clockIn->listeners.push_back(this);
    clockIn->setName("Clk");
    clockIn->setDescription("Clock-Eingang. Jedes CLOCK-Event bzw. Gate-On wird gezaehlt; Start/Stop werden durchgereicht.");

    divPin = new Pin(Pin::Type::VALUE);
    divPin->direction = Pin::Direction::IN;
    divPin->listeners.push_back(this);
    divPin->setName("N");
    divPin->setDescription("Divisor. Default 4 wenn nicht verbunden. <=0 wird auf 1 geklemmt.");

    clockOut = new Pin(Pin::Type::EVENT);
    clockOut->direction = Pin::Direction::OUT;
    clockOut->listeners.push_back(this);
    clockOut->setName("Out");
    clockOut->setDescription("Geteilter Clock-Out. Jedes N-te In-Event wird durchgereicht.");

    addPin(Pin::Direction::IN, clockIn);
    addPin(Pin::Direction::IN, divPin);
    addPin(Pin::Direction::OUT, clockOut);
}

void ClockDividerModule::paint(juce::Graphics& g) {
    Module::paint(g);
}

void ClockDividerModule::process() {
    // Clock-Logik laeuft ueber eventReceived; process() braucht nichts zu tun.
}

void ClockDividerModule::eventReceived(Event* e) {
    if (e == nullptr) return;

    const auto type = e->getType();

    // Transport-Events transparent durchreichen, damit nachgeschaltete
    // Sequencer weiterhin starten/stoppen. Bei Start den Zaehler resetten,
    // damit die Teilung sauber auf dem ersten Tick beginnt.
    if (type == Event::Type::CLOCK_START || type == Event::Type::CLOCK_STOP) {
        if (type == Event::Type::CLOCK_START) counter = 0;
        if (clockOut != nullptr) {
            for (auto* c : clockOut->getConnections())
                c->sendEvent(new Event(e));
        }
        return;
    }

    // Ein Tick ist ein echtes Clock-Event (MidiClock sendet Event::Type::CLOCK)
    // ODER ein Gate-On (Trigger-Quellen senden GATE mit Value>0). Gate-Off und
    // alles andere ignorieren, sonst wuerde man doppelt zaehlen.
    const bool isTick =
        type == Event::Type::CLOCK
        || (type == Event::Type::GATE && e->getValue() > 0);
    if (!isTick) return;

    int divisor = 4;
    if (divPin != nullptr && divPin->getConnections().size() == 1)
        divisor = static_cast<int>(std::round(divPin->getConnections().at(0)->getValue()));
    if (divisor < 1) divisor = 1;

    counter++;
    if (counter >= divisor) {
        counter = 0;
        if (clockOut != nullptr) {
            // Geteilten Tick im selben Typ ausgeben wie der Eingang: CLOCK->CLOCK
            // fuer Clock-Ketten (StepSequencer etc.), GATE->GATE fuer Trigger-/
            // Envelope-Ketten.
            Event* out = new Event("clock div out", type);
            out->setValue(e->getValue());
            out->setNote(e->getNote());
            for (auto* c : clockOut->getConnections())
                c->sendEvent(new Event(out));
            delete out;
        }
    }
}
