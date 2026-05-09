/*
  ==============================================================================

    AttenuverterModule.cpp

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "AttenuverterModule.h"

AttenuverterModule::AttenuverterModule()
{
    setSize(120, 140);
    setName("Attenuverter");

    editable = false;
    prefab = true;
}

AttenuverterModule::~AttenuverterModule()
{
}

void AttenuverterModule::configurePins() {
    // pins[0] = In  (VALUE)
    // pins[1] = Scale (VALUE) -- signed, default 1.0
    // pins[2] = Offset (VALUE) -- default 0.0
    // pins[3] = Out (VALUE)
    Pin* in = new Pin(Pin::Type::VALUE);
    in->direction = Pin::Direction::IN;
    in->listeners.push_back(this);
    in->setName("In");
    in->setDescription("Signal in (CV-Wert).");

    Pin* scale = new Pin(Pin::Type::VALUE);
    scale->direction = Pin::Direction::IN;
    scale->listeners.push_back(this);
    scale->setName("S");
    scale->setDescription("Scale (signed). Negativ = invertieren. Default 1.0 wenn nicht verbunden.");

    Pin* offset = new Pin(Pin::Type::VALUE);
    offset->direction = Pin::Direction::IN;
    offset->listeners.push_back(this);
    offset->setName("O");
    offset->setDescription("Additiver Offset. Default 0.0 wenn nicht verbunden.");

    Pin* out = new Pin(Pin::Type::VALUE);
    out->direction = Pin::Direction::OUT;
    out->listeners.push_back(this);
    out->setName("Out");
    out->setDescription("Skalierter und verschobener Wert.");

    addPin(Pin::Direction::IN, in);
    addPin(Pin::Direction::IN, scale);
    addPin(Pin::Direction::IN, offset);
    addPin(Pin::Direction::OUT, out);
}

void AttenuverterModule::paint(juce::Graphics& g) {
    Module::paint(g);
    // Kein Custom-Icon; das Modul-Default-Bild reicht.
}

void AttenuverterModule::process() {
    if (pins.size() < 4) return;

    float in = 0.0f;
    if (pins.at(0)->getConnections().size() == 1)
        in = pins.at(0)->getConnections().at(0)->getValue();

    float scale = 1.0f;
    if (pins.at(1)->getConnections().size() == 1)
        scale = pins.at(1)->getConnections().at(0)->getValue();

    float offset = 0.0f;
    if (pins.at(2)->getConnections().size() == 1)
        offset = pins.at(2)->getConnections().at(0)->getValue();

    pins.at(3)->setValue(in * scale + offset);
}
