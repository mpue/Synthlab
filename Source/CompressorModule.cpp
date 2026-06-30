/*
  ==============================================================================

    CompressorModule.cpp
    Created: 28 Jun 2026
    Author:  mpue

  ==============================================================================
*/

#include "CompressorModule.h"
#include "../JuceLibraryCode/JuceHeader.h"
#include "Connection.h"

#include <cmath>

using juce::Justification;
using juce::String;
using juce::ImageCache;

//==============================================================================
CompressorModule::CompressorModule(float sampleRate, int bufferSize)
{
    setSize(120, 140);
    setName("Compressor");
    editable = false;
    prefab = true;
    this->buffersize = bufferSize;
    this->sampleRate = sampleRate;
}

CompressorModule::~CompressorModule()
{
}

void CompressorModule::configurePins() {
    // Audio Input Pin
    audioIn = new Pin(Pin::Type::AUDIO);
    audioIn->direction = Pin::Direction::IN;
    audioIn->setName("Audio In");

    // Threshold (dB)
    thresholdIn = new Pin(Pin::Type::VALUE);
    thresholdIn->direction = Pin::Direction::IN;
    thresholdIn->setName("Threshold");

    // Ratio (n:1)
    ratioIn = new Pin(Pin::Type::VALUE);
    ratioIn->direction = Pin::Direction::IN;
    ratioIn->setName("Ratio");

    // Attack (ms)
    attackIn = new Pin(Pin::Type::VALUE);
    attackIn->direction = Pin::Direction::IN;
    attackIn->setName("Attack");

    // Release (ms)
    releaseIn = new Pin(Pin::Type::VALUE);
    releaseIn->direction = Pin::Direction::IN;
    releaseIn->setName("Release");

    // Makeup gain (dB)
    makeupIn = new Pin(Pin::Type::VALUE);
    makeupIn->direction = Pin::Direction::IN;
    makeupIn->setName("Makeup");

    // Audio Output Pin
    audioOut = new Pin(Pin::Type::AUDIO);
    audioOut->direction = Pin::Direction::OUT;
    audioOut->setName("Audio Out");

    addPin(Pin::Direction::IN, audioIn);
    audioIn->listeners.push_back(this);

    addPin(Pin::Direction::IN, thresholdIn);
    thresholdIn->listeners.push_back(this);

    addPin(Pin::Direction::IN, ratioIn);
    ratioIn->listeners.push_back(this);

    addPin(Pin::Direction::IN, attackIn);
    attackIn->listeners.push_back(this);

    addPin(Pin::Direction::IN, releaseIn);
    releaseIn->listeners.push_back(this);

    addPin(Pin::Direction::IN, makeupIn);
    makeupIn->listeners.push_back(this);

    addPin(Pin::Direction::OUT, audioOut);
    audioOut->listeners.push_back(this);

    audioOut->getAudioBuffer()->clear();
}

void CompressorModule::paint(juce::Graphics& g) {
    Module::paint(g);

    g.setColour(juce::Colours::white);

    // Draw a simple compressor transfer-curve icon (a knee)
    int x0 = 25;
    int y0 = 100;
    int x1 = getWidth() - 25;
    int y1 = 45;
    int kneeX = (x0 + x1) / 2;
    int kneeY = (y0 + y1) / 2 + 12;

    juce::Path curve;
    curve.startNewSubPath((float)x0, (float)y0);
    curve.lineTo((float)kneeX, (float)kneeY);
    curve.lineTo((float)x1, (float)y1);
    g.strokePath(curve, juce::PathStrokeType(2.0f));

    // Axes
    g.setColour(juce::Colours::grey);
    g.drawLine((float)x0, (float)y0, (float)x0, (float)y1 - 5, 1.0f);
    g.drawLine((float)x0, (float)y0, (float)x1 + 5, (float)y0, 1.0f);
}

void CompressorModule::process() {

    if (audioOut == nullptr || audioOut->getAudioBuffer() == nullptr) {
        return;
    }

    float* pOut = audioOut->getAudioBuffer()->getWritePointer(0);

    // No audio input connected -> output silence
    if (audioIn == nullptr || audioIn->getConnections().size() == 0) {
        for (int i = 0; i < buffersize; i++) {
            pOut[i] = 0.0f;
        }
        return;
    }

    const float* pIn = audioIn->getConnections().at(0)->getAudioBuffer()->getReadPointer(0);

    // Read parameters (with defaults when not connected)
    float thresholdDb = -18.0f;
    float ratio = 4.0f;
    float attackMs = 10.0f;
    float releaseMs = 100.0f;
    float makeupDb = 0.0f;

    if (thresholdIn != nullptr && thresholdIn->getConnections().size() > 0)
        thresholdDb = thresholdIn->getConnections().at(0)->getValue();
    if (ratioIn != nullptr && ratioIn->getConnections().size() > 0)
        ratio = ratioIn->getConnections().at(0)->getValue();
    if (attackIn != nullptr && attackIn->getConnections().size() > 0)
        attackMs = attackIn->getConnections().at(0)->getValue();
    if (releaseIn != nullptr && releaseIn->getConnections().size() > 0)
        releaseMs = releaseIn->getConnections().at(0)->getValue();
    if (makeupIn != nullptr && makeupIn->getConnections().size() > 0)
        makeupDb = makeupIn->getConnections().at(0)->getValue();

    // Clamp to sane ranges
    if (ratio < 1.0f) ratio = 1.0f;
    if (attackMs < 0.1f) attackMs = 0.1f;
    if (releaseMs < 1.0f) releaseMs = 1.0f;

    // Time constants -> one-pole smoothing coefficients
    const float sr = (float)sampleRate;
    const float attackCoeff = std::exp(-1.0f / (attackMs * 0.001f * sr));
    const float releaseCoeff = std::exp(-1.0f / (releaseMs * 0.001f * sr));

    const float makeupGain = std::pow(10.0f, makeupDb / 20.0f);
    const float slope = 1.0f - (1.0f / ratio);

    for (int i = 0; i < buffersize; i++) {
        const float in = pIn[i];
        const float level = std::fabs(in);

        // Peak envelope follower
        if (level > envelope)
            envelope = attackCoeff * envelope + (1.0f - attackCoeff) * level;
        else
            envelope = releaseCoeff * envelope + (1.0f - releaseCoeff) * level;

        // Gain computer in dB domain
        const float envDb = 20.0f * std::log10(envelope + 1.0e-9f);
        float gainReductionDb = 0.0f;
        if (envDb > thresholdDb)
            gainReductionDb = (envDb - thresholdDb) * slope;

        const float gain = makeupGain * std::pow(10.0f, -gainReductionDb / 20.0f);
        pOut[i] = in * gain;
    }
}
