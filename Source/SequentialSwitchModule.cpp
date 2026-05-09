/*
  ==============================================================================

    SequentialSwitchModule.cpp

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "SequentialSwitchModule.h"
#include "Connection.h"

using juce::String;
using juce::Value;
using juce::PropertyComponent;
using juce::SliderPropertyComponent;

SequentialSwitchModule::SequentialSwitchModule(double sampleRate, int buffersize)
{
    this->sampleRate = sampleRate;
    this->buffersize = buffersize;

    setSize(120, 140);
    setName("Seq Switch");

    editable = false;
    prefab = true;
}

SequentialSwitchModule::~SequentialSwitchModule()
{
    delete outputsListener;
    delete outputsValue;
}

void SequentialSwitchModule::configurePins() {
    // Festes Eingangs-Layout: In, Clk, Rst, Step. Danach Out1..OutN, wobei
    // N spaeter wachsen kann (siehe setNumOutputs). Reihenfolge ist wichtig
    // weil audioOutPinIndex() darauf basiert.
    Pin* in = new Pin(Pin::Type::AUDIO);
    in->direction = Pin::Direction::IN;
    in->listeners.push_back(this);
    in->setName("In");
    in->setDescription("Audio-Eingang. Wird in den jeweils aktiven Out kopiert.");

    Pin* clk = new Pin(Pin::Type::EVENT);
    clk->direction = Pin::Direction::IN;
    clk->listeners.push_back(this);
    clk->setName("Clk");
    clk->setDescription("Clock-Event. Rueckt den aktiven Out um 1 vor und wrappt am Ende.");

    Pin* rst = new Pin(Pin::Type::EVENT);
    rst->direction = Pin::Direction::IN;
    rst->listeners.push_back(this);
    rst->setName("Rst");
    rst->setDescription("Reset-Event. Setzt den aktiven Out auf 1 zurueck.");

    Pin* step = new Pin(Pin::Type::VALUE);
    step->direction = Pin::Direction::OUT;
    step->listeners.push_back(this);
    step->setName("Step");
    step->setDescription("Aktueller Step-Index 0..N-1 als VALUE-Out (z.B. fuer LED/Display).");

    addPin(Pin::Direction::IN,  in);
    addPin(Pin::Direction::IN,  clk);
    addPin(Pin::Direction::IN,  rst);
    addPin(Pin::Direction::OUT, step);

    for (int i = 0; i < numOutputs; ++i) {
        Pin* out = new Pin(Pin::Type::AUDIO);
        out->direction = Pin::Direction::OUT;
        out->listeners.push_back(this);
        out->setName("O" + String(i + 1));
        addPin(Pin::Direction::OUT, out);
    }

    createProperties();

    setSize(getWidth(), juce::jmax(140, 40 + 18 * numOutputs));
}

void SequentialSwitchModule::paint(juce::Graphics& g) {
    Module::paint(g);
}

int SequentialSwitchModule::audioOutPinIndex(int outIdx) const {
    // Layout: [0]=In, [1]=Clk, [2]=Rst, [3]=Step, [4..]=Out1..OutN
    return 4 + outIdx;
}

Pin* SequentialSwitchModule::getStepOutPin() const {
    return pins.at(3);
}

void SequentialSwitchModule::eventReceived(Event* e) {
    if (e == nullptr) return;
    // Wir reagieren nur auf "echte" Trigger (Value > 0 oder Clock-Event), sonst
    // wuerde jedes Note-Off durchschalten.
    const auto type = e->getType();
    const bool isTrigger =
        type == Event::Type::CLOCK
        || (type == Event::Type::GATE && e->getValue() > 0);
    if (!isTrigger) return;

    // Welcher unserer EVENT-IN-Pins hat gefeuert? Pin::sendEvent stempelt sich
    // selbst als Source ein — wir checken gegen unsere bekannten Pin-Slots.
    Pin* src = e->getSource();
    Pin* clkPin = (pins.size() > 1) ? pins.at(1) : nullptr;
    Pin* rstPin = (pins.size() > 2) ? pins.at(2) : nullptr;

    if (src == rstPin && rstPin != nullptr) {
        currentStep.store(0);
        getStepOutPin()->setValue(0.0f);
        return;
    }
    if (src == clkPin || src == nullptr) {
        // src==nullptr: Fallback fuer Events die nicht via Pin::sendEvent
        // geschickt werden — sicherheitshalber als Clock interpretieren.
        int next = (currentStep.load() + 1) % juce::jmax(1, numOutputs);
        currentStep.store(next);
        getStepOutPin()->setValue((float) next);
    }
}

void SequentialSwitchModule::clearOutputs(int sampleCount) {
    for (int i = 0; i < numOutputs; ++i) {
        Pin* op = pins.at(audioOutPinIndex(i));
        juce::AudioSampleBuffer* buf = op->getAudioBuffer();
        if (buf == nullptr || buf->getNumChannels() == 0) continue;
        float* w = buf->getWritePointer(0);
        for (int j = 0; j < sampleCount; ++j) w[j] = 0.0f;
    }
}

void SequentialSwitchModule::writeActiveOutput(const float* in, int sampleCount) {
    const int active = juce::jlimit(0, numOutputs - 1, currentStep.load());
    Pin* op = pins.at(audioOutPinIndex(active));
    juce::AudioSampleBuffer* buf = op->getAudioBuffer();
    if (buf == nullptr || buf->getNumChannels() == 0) return;
    float* w = buf->getWritePointer(0);
    for (int j = 0; j < sampleCount; ++j) w[j] = in[j];
}

void SequentialSwitchModule::process() {
    if (numOutputs <= 0 || buffersize <= 0) return;

    // Erst alle Outputs leerschreiben (sonst dudeln die nicht-aktiven mit
    // dem Vorblock-Garbage weiter), dann den aktiven mit dem Eingang fuellen.
    clearOutputs(buffersize);

    Pin* inPin = pins.at(0);
    if (inPin->getConnections().size() != 1) return;

    juce::AudioSampleBuffer* inBuf = inPin->getConnections().at(0)->getAudioBuffer();
    if (inBuf == nullptr || inBuf->getNumChannels() == 0) return;

    writeActiveOutput(inBuf->getReadPointer(0), buffersize);
}

void SequentialSwitchModule::setNumOutputs(int newCount) {
    newCount = juce::jlimit(2, kMaxOutputs, newCount);
    if (newCount <= numOutputs) return;     // schrumpfen wuerde Connections kaputtmachen

    for (int i = numOutputs; i < newCount; ++i) {
        Pin* out = new Pin(Pin::Type::AUDIO);
        out->direction = Pin::Direction::OUT;
        out->listeners.push_back(this);
        out->setName("O" + String(i + 1));
        addPin(Pin::Direction::OUT, out);
    }

    numOutputs = newCount;
    setSize(getWidth(), juce::jmax(140, 40 + 18 * numOutputs));
    repaint();
}

void SequentialSwitchModule::createProperties() {
    outputsValue = new Value();
    outputsListener = new OutputsListener(*outputsValue, this);
    outputsValue->setValue(numOutputs);
}

juce::Array<PropertyComponent*>& SequentialSwitchModule::getProperties() {
    properties = juce::Array<PropertyComponent*>();
    outputsProp = new SliderPropertyComponent(*outputsValue, "Outputs", 2, kMaxOutputs, 1, 1, true);
    properties.add(outputsProp);
    return properties;
}
