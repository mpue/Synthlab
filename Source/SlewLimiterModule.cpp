/*
  ==============================================================================

    SlewLimiterModule.cpp

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "SlewLimiterModule.h"

SlewLimiterModule::SlewLimiterModule(double sampleRate, int buffersize)
{
    this->sampleRate = sampleRate;
    this->buffersize = buffersize;

    setSize(120, 140);
    setName("Slew");

    editable = false;
    prefab = true;
}

SlewLimiterModule::~SlewLimiterModule()
{
}

void SlewLimiterModule::configurePins() {
    // pins[0] = In  (VALUE)
    // pins[1] = Rise time (seconds, default 0.1)
    // pins[2] = Fall time (seconds, default 0.1)
    // pins[3] = Out (VALUE)
    Pin* in = new Pin(Pin::Type::VALUE);
    in->direction = Pin::Direction::IN;
    in->listeners.push_back(this);
    in->setName("In");
    in->setDescription("Eingangs-CV.");

    Pin* rise = new Pin(Pin::Type::VALUE);
    rise->direction = Pin::Direction::IN;
    rise->listeners.push_back(this);
    rise->setName("R");
    rise->setDescription("Rise-Zeit in Sekunden pro Einheit. Default 0.1 wenn nicht verbunden.");

    Pin* fall = new Pin(Pin::Type::VALUE);
    fall->direction = Pin::Direction::IN;
    fall->listeners.push_back(this);
    fall->setName("F");
    fall->setDescription("Fall-Zeit in Sekunden pro Einheit. Default 0.1 wenn nicht verbunden.");

    Pin* out = new Pin(Pin::Type::VALUE);
    out->direction = Pin::Direction::OUT;
    out->listeners.push_back(this);
    out->setName("Out");
    out->setDescription("Geslewter Wert.");

    addPin(Pin::Direction::IN, in);
    addPin(Pin::Direction::IN, rise);
    addPin(Pin::Direction::IN, fall);
    addPin(Pin::Direction::OUT, out);
}

void SlewLimiterModule::paint(juce::Graphics& g) {
    Module::paint(g);
}

void SlewLimiterModule::process() {
    if (pins.size() < 4 || sampleRate <= 0.0 || buffersize <= 0) return;

    float in = currentOut;
    if (pins.at(0)->getConnections().size() == 1)
        in = pins.at(0)->getConnections().at(0)->getValue();

    float riseT = 0.1f;
    if (pins.at(1)->getConnections().size() == 1)
        riseT = pins.at(1)->getConnections().at(0)->getValue();

    float fallT = 0.1f;
    if (pins.at(2)->getConnections().size() == 1)
        fallT = pins.at(2)->getConnections().at(0)->getValue();

    // Schutzklammer — bei 0 oder negativ gibt's Division-by-zero / runaway.
    if (riseT < 1.0e-4f) riseT = 1.0e-4f;
    if (fallT < 1.0e-4f) fallT = 1.0e-4f;

    const float blockSec = static_cast<float>(buffersize) / static_cast<float>(sampleRate);
    const float delta = in - currentOut;

    if (delta > 0.0f) {
        const float maxStep = blockSec / riseT;
        currentOut += std::min(delta, maxStep);
    } else if (delta < 0.0f) {
        const float maxStep = -blockSec / fallT;
        currentOut += std::max(delta, maxStep);
    }

    pins.at(3)->setValue(currentOut);
}
