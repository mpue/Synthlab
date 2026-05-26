/*
  ==============================================================================

    BinaryCounterModule.cpp

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "BinaryCounterModule.h"
#include "Connection.h"

using juce::String;
using juce::Value;
using juce::PropertyComponent;
using juce::SliderPropertyComponent;

BinaryCounterModule::BinaryCounterModule()
{
    setSize(120, 140);
    setName("Bin Cnt");

    editable = false;
    prefab = true;
}

BinaryCounterModule::~BinaryCounterModule()
{
    delete bitsListener;
    delete bitsValue;
}

void BinaryCounterModule::configurePins() {
    // Festes Eingangs-Layout: Clk, Rst, dann Val (Dezimal-Out), dann B0..Bn-1.
    // bitOutPinIndex() basiert auf dieser Reihenfolge.
    Pin* clk = new Pin(Pin::Type::EVENT);
    clk->direction = Pin::Direction::IN;
    clk->listeners.push_back(this);
    clk->setName("Clk");
    clk->setDescription("Clock-Event. Erhoeht den Zaehler um 1 (nur Gate-On).");

    Pin* rst = new Pin(Pin::Type::EVENT);
    rst->direction = Pin::Direction::IN;
    rst->listeners.push_back(this);
    rst->setName("Rst");
    rst->setDescription("Reset-Event. Setzt den Zaehler auf 0.");

    Pin* val = new Pin(Pin::Type::VALUE);
    val->direction = Pin::Direction::OUT;
    val->listeners.push_back(this);
    val->setName("Val");
    val->setDescription("Aktueller Dezimalwert 0..(2^Bits-1) als VALUE-Out.");

    addPin(Pin::Direction::IN,  clk);
    addPin(Pin::Direction::IN,  rst);
    addPin(Pin::Direction::OUT, val);

    for (int i = 0; i < numBits; ++i) {
        Pin* b = new Pin(Pin::Type::VALUE);
        b->direction = Pin::Direction::OUT;
        b->listeners.push_back(this);
        b->setName("B" + String(i));
        b->setDescription("Bit " + String(i) + " (Wertigkeit " + String(1 << i) + ")");
        addPin(Pin::Direction::OUT, b);
    }

    createProperties();

    setSize(getWidth(), juce::jmax(140, 40 + 18 * (numBits + 1)));
}

void BinaryCounterModule::paint(juce::Graphics& g) {
    Module::paint(g);
}

int BinaryCounterModule::bitOutPinIndex(int bitIdx) const {
    // Layout: [0]=Clk, [1]=Rst, [2]=Val, [3..]=B0..Bn-1
    return 3 + bitIdx;
}

void BinaryCounterModule::eventReceived(Event* e) {
    if (e == nullptr) return;

    const auto type = e->getType();
    const bool isTrigger =
        type == Event::Type::CLOCK
        || (type == Event::Type::GATE && e->getValue() > 0);
    if (!isTrigger) return;

    Pin* src = e->getSource();
    Pin* clkPin = (pins.size() > 0) ? pins.at(0) : nullptr;
    Pin* rstPin = (pins.size() > 1) ? pins.at(1) : nullptr;

    if (src == rstPin && rstPin != nullptr) {
        counter.store(0);
        return;
    }
    if (src == clkPin || src == nullptr) {
        // src==nullptr: Fallback fuer Events ohne Source-Stempel
        // (Pin::sendEvent setzt sich normalerweise selbst ein) — sicherheitshalber
        // als Clock interpretieren.
        const int mod = 1 << juce::jlimit(1, kMaxBits, numBits);
        counter.store((counter.load() + 1) % mod);
    }
}

void BinaryCounterModule::process() {
    if (pins.size() < 3) return;

    const int c = counter.load();

    // Dezimal-Out
    pins.at(2)->setValue((float) c);

    // Bit-Outs
    for (int i = 0; i < numBits; ++i) {
        const int idx = bitOutPinIndex(i);
        if (idx >= (int) pins.size()) break;
        const float bit = (c >> i) & 0x1 ? 1.0f : 0.0f;
        pins.at(idx)->setValue(bit);
    }
}

void BinaryCounterModule::setNumBits(int newBits) {
    newBits = juce::jlimit(1, kMaxBits, newBits);
    if (newBits <= numBits) return;     // schrumpfen wuerde Connections kaputtmachen

    for (int i = numBits; i < newBits; ++i) {
        Pin* b = new Pin(Pin::Type::VALUE);
        b->direction = Pin::Direction::OUT;
        b->listeners.push_back(this);
        b->setName("B" + String(i));
        b->setDescription("Bit " + String(i) + " (Wertigkeit " + String(1 << i) + ")");
        addPin(Pin::Direction::OUT, b);
    }

    numBits = newBits;

    // Wrap-around an neue Bit-Breite anpassen, falls der bisherige Stand
    // ueber dem alten Maximum lag (kann hier nie passieren weil wir nur
    // wachsen, aber falls jemand spaeter Schrumpfen einbaut: hier clampen).
    const int mod = 1 << numBits;
    if (counter.load() >= mod) counter.store(0);

    setSize(getWidth(), juce::jmax(140, 40 + 18 * (numBits + 1)));
    repaint();
}

void BinaryCounterModule::createProperties() {
    bitsValue = new Value();
    bitsListener = new BitsListener(*bitsValue, this);
    bitsValue->setValue(numBits);
}

juce::Array<PropertyComponent*>& BinaryCounterModule::getProperties() {
    properties = juce::Array<PropertyComponent*>();
    bitsProp = new SliderPropertyComponent(*bitsValue, "Bits", 1, kMaxBits, 1, 1, true);
    properties.add(bitsProp);
    return properties;
}
