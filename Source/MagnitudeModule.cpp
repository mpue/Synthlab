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
    g.drawImageAt(ImageCache::getFromMemory(BinaryData::magnitue_png, BinaryData::magnitue_pngSize), 25, 40);
}

void MagnitudeModule::process()
{
    if (pins.at(0)->getConnections().size() == 1) {
        juce::AudioSampleBuffer* buffer = pins.at(0)->getConnections().at(0)->getAudioBuffer();

        if (buffer == nullptr || buffer->getNumSamples() == 0)
            return;

        // Pin-Audiobuffer sind fix auf 1024 Samples alloziert und werden NICHT auf
        // die Blockgroesse verkleinert — pro Block befuellt die Engine nur
        // [0..buffersize). Daher ab Sample 0 ueber die echte Blockgroesse messen
        // (wie AdderModule), nicht ueber buffer->getNumSamples() (= Kapazitaet):
        // sonst landet das Messfenster in der nie geschriebenen Region am Ende und
        // die Magnitude bleibt ~0.
        const int numSamples = juce::jmin (buffersize, buffer->getNumSamples());

        magnitude = buffer->getMagnitude(0, 0, numSamples);
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
