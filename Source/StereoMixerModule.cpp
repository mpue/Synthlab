/*
 ==============================================================================

 StereoMixerModule.cpp

 ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "StereoMixerModule.h"
#include "Connection.h"

using juce::Justification;
using juce::String;
using juce::ImageCache;
using juce::Value;
using juce::PropertyComponent;
using juce::Array;
using juce::SliderPropertyComponent;

StereoMixerModule::StereoMixerModule(float sampleRate, int buffersize)
{
    // Each channel has 3 IN pins (InL, InR, A) at 20 px each = 60 px per channel.
    setSize(140, 60 * numInputs + 20);
    setName("Stereo Mixer");
    editable = false;
    prefab = true;
    this->buffersize = buffersize;
    this->sampleRate = sampleRate;
    createProperties();
}

StereoMixerModule::~StereoMixerModule()
{
    delete inputsValue;
    delete inputsListener;
}

void StereoMixerModule::appendChannel(int idx) {
    Pin* pL = new Pin(Pin::Type::AUDIO);
    pL->direction = Pin::Direction::IN;
    pL->setName("InL" + String(idx + 1));
    addPin(Pin::Direction::IN, pL);
    pL->listeners.push_back(this);

    Pin* pR = new Pin(Pin::Type::AUDIO);
    pR->direction = Pin::Direction::IN;
    pR->setName("InR" + String(idx + 1));
    addPin(Pin::Direction::IN, pR);
    pR->listeners.push_back(this);

    Pin* pA = new Pin(Pin::Type::VALUE);
    pA->direction = Pin::Direction::IN;
    pA->setName("A" + String(idx + 1));
    addPin(Pin::Direction::IN, pA);
    pA->listeners.push_back(this);
}

void StereoMixerModule::configurePins() {
    for (int i = 0; i < numInputs; ++i)
        appendChannel(i);

    outL = new Pin(Pin::Type::AUDIO);
    outL->direction = Pin::Direction::OUT;
    outL->setName("OutL");
    outL->listeners.push_back(this);
    addPin(Pin::Direction::OUT, outL);
    outL->getAudioBuffer()->clear();

    outR = new Pin(Pin::Type::AUDIO);
    outR->direction = Pin::Direction::OUT;
    outR->setName("OutR");
    outR->listeners.push_back(this);
    addPin(Pin::Direction::OUT, outR);
    outR->getAudioBuffer()->clear();

    inputsValue->setValue(numInputs);
}

void StereoMixerModule::paint(juce::Graphics& g) {
    Module::paint(g);
    g.drawImageAt(ImageCache::getFromMemory(BinaryData::mixer_png, BinaryData::mixer_pngSize), 25, 40);
}

void StereoMixerModule::process() {
    if (outL == nullptr || outL->getAudioBuffer() == nullptr ||
        outR == nullptr || outR->getAudioBuffer() == nullptr)
        return;

    float* pOutL = outL->getAudioBuffer()->getWritePointer(0);
    float* pOutR = outR->getAudioBuffer()->getWritePointer(0);

    for (int j = 0; j < buffersize; ++j) {
        float sumL = 0.0f;
        float sumR = 0.0f;

        // Walk the pin list looking for (AUDIO IN, AUDIO IN, VALUE IN) triples.
        // setInputs() appends new triples after the OUT pins, so this scan is
        // resilient to dynamic input growth — same idea as MixerModule's
        // type/direction-based skip of the OUT pin.
        for (int i = 0; i + 2 < pins.size(); ) {
            Pin* pL = pins.at(i);
            Pin* pR = pins.at(i + 1);
            Pin* pA = pins.at(i + 2);
            const bool triple =
                pL->direction == Pin::Direction::IN && pL->getType() == Pin::Type::AUDIO &&
                pR->direction == Pin::Direction::IN && pR->getType() == Pin::Type::AUDIO &&
                pA->direction == Pin::Direction::IN && pA->getType() == Pin::Type::VALUE;

            if (!triple) { ++i; continue; }

            float gain = 0.0f;
            if (pA->getConnections().size() == 1)
                gain = pA->getConnections().at(0)->getValue();

            if (pL->getConnections().size() == 1) {
                const float* in = pL->getConnections().at(0)->getAudioBuffer()->getReadPointer(0);
                sumL += in[j] * gain;
            }
            if (pR->getConnections().size() == 1) {
                const float* in = pR->getConnections().at(0)->getAudioBuffer()->getReadPointer(0);
                sumR += in[j] * gain;
            }

            i += 3;
        }

        pOutL[j] = sumL;
        pOutR[j] = sumR;
    }
}

void StereoMixerModule::setInputs(int newInputs) {
    if (newInputs > this->numInputs) {
        for (int i = numInputs; i < newInputs; ++i)
            appendChannel(i);

        numInputs = newInputs;
        setSize(getWidth(), 60 * numInputs + 20);
        repaint();
    }
}

void StereoMixerModule::createProperties() {
    inputsValue = new Value();
    inputsListener = new InputsListener(*inputsValue, this);
}

juce::Array<PropertyComponent*>& StereoMixerModule::getProperties() {
    properties = juce::Array<PropertyComponent*>();
    inputsProp = new SliderPropertyComponent(*inputsValue, "Inputs", 1, 16, 1, 1, true);
    properties.add(inputsProp);
    return properties;
}
