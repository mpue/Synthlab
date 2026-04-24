/*
  ==============================================================================

    AudioInSelector.cpp
    Created: 2026
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "AudioInSelector.h"
#include "Connection.h"

using juce::AudioSampleBuffer;
using juce::ImageCache;

//==============================================================================
AudioInSelector::AudioInSelector(int bufferSize)
{
    setSize(120, 160);
    setName("Audio selector");
    editable = false;
    prefab = true;
    this->buffersize = bufferSize;
}

AudioInSelector::~AudioInSelector()
{
}

void AudioInSelector::configurePins()
{
    // Pin 0: value selector input
    Pin* sel = new Pin(Pin::Type::VALUE);
    sel->direction = Pin::Direction::IN;
    sel->setName("Sel");
    addPin(Pin::Direction::IN, sel);

    // Pins 1-3: audio inputs A, B, C
    const char* names[] = { "A", "B", "C" };
    for (int i = 0; i < 3; ++i) {
        Pin* p = new Pin(Pin::Type::AUDIO);
        p->direction = Pin::Direction::IN;
        p->setName(names[i]);
        addPin(Pin::Direction::IN, p);
    }

    // Pin 4: audio output
    Pin* out = new Pin(Pin::Type::AUDIO);
    out->direction = Pin::Direction::OUT;
    out->setName("Out");
    addPin(Pin::Direction::OUT, out);
}

void AudioInSelector::paint(juce::Graphics& g)
{
    Module::paint(g);
    g.drawImageAt(ImageCache::getFromMemory(BinaryData::module_png, BinaryData::module_pngSize), 28, 40);
}

void AudioInSelector::process()
{
    // Determine selected input (0, 1 or 2)
    int selected = 0;
    if (pins.at(0)->getConnections().size() > 0) {
        float val = pins.at(0)->getConnections().at(0)->getValue();
        selected = juce::jlimit(0, 2, static_cast<int>(std::round(val)));
    }

    // Pin index of the chosen audio input (1=A, 2=B, 3=C)
    int srcPinIdx = 1 + selected;

    Pin* outPin = pins.at(4);
    float* out = outPin->getAudioBuffer()->getWritePointer(0);

    if (pins.at(srcPinIdx)->getConnections().size() > 0) {
        AudioSampleBuffer* srcBuf = pins.at(srcPinIdx)->getConnections().at(0)->getAudioBuffer();
        if (srcBuf != nullptr) {
            const float* in = srcBuf->getReadPointer(0);
            for (int i = 0; i < buffersize; ++i)
                out[i] = in[i];
            return;
        }
    }

    // No connection on selected input: output silence
    outPin->getAudioBuffer()->clear();
}
