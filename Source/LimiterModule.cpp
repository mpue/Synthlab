/*
  ==============================================================================

    LimiterModule.cpp
    Created: 28 Jun 2026
    Author:  mpue

  ==============================================================================
*/

#include "LimiterModule.h"
#include "../JuceLibraryCode/JuceHeader.h"
#include "Connection.h"

#include <cmath>

using juce::Justification;
using juce::String;
using juce::ImageCache;

//==============================================================================
LimiterModule::LimiterModule(float sampleRate, int bufferSize)
{
    setSize(120, 140);
    setName("Limiter");
    editable = false;
    prefab = true;
    this->buffersize = bufferSize;
    this->sampleRate = sampleRate;
}

LimiterModule::~LimiterModule()
{
}

void LimiterModule::configurePins() {
    // Audio Input Pin
    audioIn = new Pin(Pin::Type::AUDIO);
    audioIn->direction = Pin::Direction::IN;
    audioIn->setName("Audio In");

    // Ceiling (dB)
    ceilingIn = new Pin(Pin::Type::VALUE);
    ceilingIn->direction = Pin::Direction::IN;
    ceilingIn->setName("Ceiling");

    // Release (ms)
    releaseIn = new Pin(Pin::Type::VALUE);
    releaseIn->direction = Pin::Direction::IN;
    releaseIn->setName("Release");

    // Audio Output Pin
    audioOut = new Pin(Pin::Type::AUDIO);
    audioOut->direction = Pin::Direction::OUT;
    audioOut->setName("Audio Out");

    addPin(Pin::Direction::IN, audioIn);
    audioIn->listeners.push_back(this);

    addPin(Pin::Direction::IN, ceilingIn);
    ceilingIn->listeners.push_back(this);

    addPin(Pin::Direction::IN, releaseIn);
    releaseIn->listeners.push_back(this);

    addPin(Pin::Direction::OUT, audioOut);
    audioOut->listeners.push_back(this);

    audioOut->getAudioBuffer()->clear();
}

void LimiterModule::paint(juce::Graphics& g) {
    Module::paint(g);

    g.setColour(juce::Colours::white);

    int x0 = 25;
    int y0 = 100;
    int x1 = getWidth() - 25;

    // Ceiling line
    int ceilY = 55;

    // Draw an input ramp that gets clipped at the ceiling
    juce::Path curve;
    curve.startNewSubPath((float)x0, (float)y0);
    int kneeX = x0 + (int)((x1 - x0) * 0.55f);
    curve.lineTo((float)kneeX, (float)ceilY);
    curve.lineTo((float)x1, (float)ceilY);
    g.strokePath(curve, juce::PathStrokeType(2.0f));

    // Dashed ceiling line
    g.setColour(juce::Colours::orange);
    float dashes[] = { 4.0f, 3.0f };
    juce::Path ceiling;
    ceiling.startNewSubPath((float)x0, (float)ceilY);
    ceiling.lineTo((float)x1 + 5, (float)ceilY);
    juce::PathStrokeType(1.0f).createDashedStroke(ceiling, ceiling, dashes, 2);
    g.strokePath(ceiling, juce::PathStrokeType(1.0f));

    // Axes
    g.setColour(juce::Colours::grey);
    g.drawLine((float)x0, (float)y0, (float)x0, (float)ceilY - 10, 1.0f);
    g.drawLine((float)x0, (float)y0, (float)x1 + 5, (float)y0, 1.0f);
}

void LimiterModule::process() {

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
    float ceilingDb = -0.3f;
    float releaseMs = 50.0f;

    if (ceilingIn != nullptr && ceilingIn->getConnections().size() > 0)
        ceilingDb = ceilingIn->getConnections().at(0)->getValue();
    if (releaseIn != nullptr && releaseIn->getConnections().size() > 0)
        releaseMs = releaseIn->getConnections().at(0)->getValue();

    if (releaseMs < 1.0f) releaseMs = 1.0f;

    const float sr = (float)sampleRate;
    const float releaseCoeff = std::exp(-1.0f / (releaseMs * 0.001f * sr));
    const float ceilingLin = std::pow(10.0f, ceilingDb / 20.0f);

    for (int i = 0; i < buffersize; i++) {
        const float in = pIn[i];
        const float level = std::fabs(in);

        // Gain required to keep this sample under the ceiling
        float targetGain = 1.0f;
        if (level > ceilingLin)
            targetGain = ceilingLin / level;

        // Instantaneous attack (clamp immediately), smoothed release
        if (targetGain < gain)
            gain = targetGain;
        else
            gain = releaseCoeff * gain + (1.0f - releaseCoeff) * targetGain;

        pOut[i] = in * gain;
    }
}
