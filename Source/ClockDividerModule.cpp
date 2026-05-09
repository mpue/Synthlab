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
    clockIn->setDescription("Clock-Eingang. Jedes Gate-On-Event wird gezaehlt.");

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
    if (e == nullptr || e->getType() != Event::Type::GATE) return;
    // Nur Gate-On zaehlen; Gate-Off ignorieren (sonst wuerde man bei jeder
    // Note-On/Off-Paarung doppelt tickern).
    if (e->getValue() <= 0) return;

    int divisor = 4;
    if (divPin != nullptr && divPin->getConnections().size() == 1)
        divisor = static_cast<int>(std::round(divPin->getConnections().at(0)->getValue()));
    if (divisor < 1) divisor = 1;

    counter++;
    if (counter >= divisor) {
        counter = 0;
        if (clockOut != nullptr) {
            Event* out = new Event("clock div out", Event::Type::GATE);
            out->setValue(e->getValue());
            out->setNote(e->getNote());
            for (auto* c : clockOut->getConnections())
                c->sendEvent(new Event(out));
            delete out;
        }
    }
}
