/*
  ==============================================================================

    BinaryToDecimalModule.cpp

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "BinaryToDecimalModule.h"
#include "Connection.h"

using juce::String;
using juce::Value;
using juce::PropertyComponent;
using juce::SliderPropertyComponent;

BinaryToDecimalModule::BinaryToDecimalModule()
{
    setSize(120, 140);
    setName("Bin>Dec");

    editable = false;
    prefab = true;
}

BinaryToDecimalModule::~BinaryToDecimalModule()
{
    delete bitsListener;
    delete bitsValue;
}

void BinaryToDecimalModule::configurePins() {
    // Festes Layout: Val (Dezimal-Out) auf Index 0, danach B0..Bn-1 (Bit-Ins).
    // bitInPinIndex() basiert auf dieser Reihenfolge; Val bleibt beim Wachsen
    // der Bits stabil auf Index 0.
    Pin* val = new Pin(Pin::Type::VALUE);
    val->direction = Pin::Direction::OUT;
    val->listeners.push_back(this);
    val->setName("Val");
    val->setDescription("Dezimalwert 0..(2^Bits-1) als VALUE-Out (sum(B_i * 2^i)).");
    addPin(Pin::Direction::OUT, val);

    for (int i = 0; i < numBits; ++i) {
        Pin* b = new Pin(Pin::Type::VALUE);
        b->direction = Pin::Direction::IN;
        b->listeners.push_back(this);
        b->setName("B" + String(i));
        b->setDescription("Bit " + String(i) + " (Wertigkeit " + String(1 << i)
                          + ") — >= 0.5 zaehlt als 1.");
        addPin(Pin::Direction::IN, b);
    }

    createProperties();

    setSize(getWidth(), juce::jmax(140, 40 + 18 * (numBits + 1)));
}

void BinaryToDecimalModule::paint(juce::Graphics& g) {
    Module::paint(g);

    g.setColour(juce::Colours::white);
    g.setFont(16.0f);
    g.drawText(String(lastValue.load()),
               getWidth() / 3, getHeight() / 2 - 10,
               (int)(getWidth() * 0.6), 20,
               juce::Justification::centred);
}

void BinaryToDecimalModule::process() {
    if (pins.size() < 1) return;

    int value = 0;
    for (int i = 0; i < numBits; ++i) {
        const int idx = bitInPinIndex(i);
        if (idx >= (int) pins.size()) break;

        Pin* p = pins.at(idx);
        if (p->getConnections().size() > 0) {
            const float v = p->getConnections().at(0)->getValue();
            if (v >= 0.5f)
                value |= (1 << i);
        }
    }

    lastValue.store(value);
    pins.at(0)->setValue((float) value);
}

void BinaryToDecimalModule::setNumBits(int newBits) {
    newBits = juce::jlimit(1, kMaxBits, newBits);
    if (newBits <= numBits) return;     // schrumpfen wuerde Connections kaputtmachen

    for (int i = numBits; i < newBits; ++i) {
        Pin* b = new Pin(Pin::Type::VALUE);
        b->direction = Pin::Direction::IN;
        b->listeners.push_back(this);
        b->setName("B" + String(i));
        b->setDescription("Bit " + String(i) + " (Wertigkeit " + String(1 << i)
                          + ") — >= 0.5 zaehlt als 1.");
        addPin(Pin::Direction::IN, b);
    }

    numBits = newBits;

    setSize(getWidth(), juce::jmax(140, 40 + 18 * (numBits + 1)));
    repaint();
}

void BinaryToDecimalModule::createProperties() {
    bitsValue = new Value();
    bitsListener = new BitsListener(*bitsValue, this);
    bitsValue->setValue(numBits);
}

juce::Array<PropertyComponent*>& BinaryToDecimalModule::getProperties() {
    properties = juce::Array<PropertyComponent*>();
    bitsProp = new SliderPropertyComponent(*bitsValue, "Bits", 1, kMaxBits, 1, 1, true);
    properties.add(bitsProp);
    return properties;
}
