/*
  ==============================================================================

    MagnitudeModule.cpp
    Created: 7 Oct 2025 12:47:23pm
    Author:  mpue

  ==============================================================================
*/

#include "MagnitudeModule.h"

MagnitudeModule::MagnitudeModule()
{
    setSize(120, 140);
    setName("Magnitude");
    editable = false;
    prefab = true;
    setInterceptsMouseClicks(false, true);
}

MagnitudeModule::~MagnitudeModule()
{
}

void MagnitudeModule::paint(juce::Graphics& g)
{
    Module::paint(g);
}

void MagnitudeModule::process()
{
    if (pins.at(0)->getConnections().size() == 1) {
        juce::AudioSampleBuffer* buffer = pins.at(0)->getConnections().at(0)->getAudioBuffer();

        // Nur einen Teil des Buffers verarbeiten (z.B. letzte 512 Samples)
        const int chunkSize = 512;
        const int numSamples = buffer->getNumSamples();
        const int startSample = juce::jmax(0, numSamples - chunkSize);
        const int samplesToProcess = numSamples - startSample;

        magnitude = buffer->getMagnitude(0, startSample, samplesToProcess);
        pins.at(1)->setValue(magnitude);
    }
}

void MagnitudeModule::configurePins()
{
    Pin* p1 = new Pin(Pin::Type::AUDIO);
    p1->direction = Pin::Direction::IN;
    p1->setName("In");

    Pin* p2 = new Pin(Pin::Type::VALUE);
    p2->direction = Pin::Direction::OUT;
    p2->setName("Mag");

    addPin(p1->direction, p1);
    addPin(p2->direction, p2);
}
