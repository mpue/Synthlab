/*
  ==============================================================================

    TFlipFlopModule.cpp

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "TFlipFlopModule.h"
#include "Connection.h"

TFlipFlopModule::TFlipFlopModule()
{
    setSize(120, 100);
    setName("T-FF");

    editable = false;
    prefab = true;
}

TFlipFlopModule::~TFlipFlopModule()
{
}

void TFlipFlopModule::configurePins() {
    // Layout: [0]=T (Event In), [1]=Tv (Value In, Rising-Edge),
    //         [2]=Rst (Event In), [3]=Rv (Value In, Rising-Edge),
    //         [4]=Q (Value Out), [5]=/Q (Value Out).
    // Tv/Rv ermoeglichen das Verketten zweier T-FFs (Q[n] -> Tv[n+1]) ohne
    // Value-To-Event-Konverter dazwischen.
    Pin* t = new Pin(Pin::Type::EVENT);
    t->direction = Pin::Direction::IN;
    t->listeners.push_back(this);
    t->setName("T");
    t->setDescription("Toggle-Event. Bei jedem Gate-On wird Q invertiert.");

    Pin* tv = new Pin(Pin::Type::VALUE);
    tv->direction = Pin::Direction::IN;
    tv->listeners.push_back(this);
    tv->setName("Tv");
    tv->setDescription("Toggle-Value. Rising-Edge (0->1) invertiert Q. "
                       "Zum Chainen: Q[n] -> Tv[n+1] = halbierter Takt.");

    Pin* rst = new Pin(Pin::Type::EVENT);
    rst->direction = Pin::Direction::IN;
    rst->listeners.push_back(this);
    rst->setName("Rst");
    rst->setDescription("Reset-Event. Setzt Q auf 0.");

    Pin* rv = new Pin(Pin::Type::VALUE);
    rv->direction = Pin::Direction::IN;
    rv->listeners.push_back(this);
    rv->setName("Rv");
    rv->setDescription("Reset-Value. Rising-Edge (0->1) setzt Q auf 0.");

    Pin* q = new Pin(Pin::Type::VALUE);
    q->direction = Pin::Direction::OUT;
    q->listeners.push_back(this);
    q->setName("Q");
    q->setDescription("Aktueller Zustand 0 oder 1.");

    Pin* nq = new Pin(Pin::Type::VALUE);
    nq->direction = Pin::Direction::OUT;
    nq->listeners.push_back(this);
    nq->setName("/Q");
    nq->setDescription("Komplement von Q.");

    addPin(Pin::Direction::IN,  t);
    addPin(Pin::Direction::IN,  tv);
    addPin(Pin::Direction::IN,  rst);
    addPin(Pin::Direction::IN,  rv);
    addPin(Pin::Direction::OUT, q);
    addPin(Pin::Direction::OUT, nq);
}

void TFlipFlopModule::paint(juce::Graphics& g) {
    Module::paint(g);
}

void TFlipFlopModule::eventReceived(Event* e) {
    if (e == nullptr) return;

    // Spiegelt das Trigger-Kriterium von BinaryCounterModule, damit der T-FF
    // an denselben Clock-Quellen haengt (MidiClock-CLOCK-Events und
    // Gate-On-Pulse).
    const auto type = e->getType();
    const bool isTrigger =
        type == Event::Type::CLOCK
        || (type == Event::Type::GATE && e->getValue() > 0);
    if (!isTrigger) return;

    Pin* src = e->getSource();
    Pin* tPin   = (pins.size() > 0) ? pins.at(0) : nullptr;
    Pin* rstPin = (pins.size() > 2) ? pins.at(2) : nullptr;

    if (src == rstPin && rstPin != nullptr) {
        state.store(false);
        return;
    }
    if (src == tPin || src == nullptr) {
        // src==nullptr: Fallback fuer Events ohne Source-Stempel, als Clock
        // interpretieren (gleicher Pfad wie BinaryCounterModule).
        state.store(!state.load());
    }
}

void TFlipFlopModule::process() {
    if (pins.size() < 6) return;

    // VALUE-In: ueber die Connection des Pins den aktuellen Wert der Source
    // lesen (Standard-Pattern, siehe AndModule/OrModule). pin->getValue() auf
    // einem IN-Pin liefert nur den Default — die Verbindung wird nicht
    // automatisch in das Pin gespiegelt.
    auto readValuePin = [](Pin* p) -> float {
        if (p == nullptr) return 0.0f;
        auto& conns = p->getConnections();
        if (conns.size() > 0 && conns.at(0) != nullptr)
            return conns.at(0)->getValue();
        return 0.0f;
    };

    const float tv = readValuePin(pins.at(1));
    const float rv = readValuePin(pins.at(3));

    // Rising-Edge auf Tv/Rv. Schwellwert 0.5 mittig, damit nicht-saubere
    // 0/1-Signale nicht mehrfach pro logischer Flanke triggern.
    if (rv >= 0.5f && lastRstValue < 0.5f) {
        state.store(false);
    }
    else if (tv >= 0.5f && lastTValue < 0.5f) {
        state.store(!state.load());
    }

    lastTValue = tv;
    lastRstValue = rv;

    const float q = state.load() ? 1.0f : 0.0f;
    pins.at(4)->setValue(q);
    pins.at(5)->setValue(1.0f - q);
}
