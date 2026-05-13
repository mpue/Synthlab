/*
 ==============================================================================

 BusOut.cpp

 ==============================================================================
 */

#include "../JuceLibraryCode/JuceHeader.h"
#include "BusOut.h"

using juce::Justification;
using juce::ImageCache;

BusOut::BusOut()
{
    setSize(120, 140);
    nameLabel->setJustificationType(Justification::left);
    nameLabel->setTopLeftPosition(18, 2);
    setName("Bus out");

    editable = false;
    prefab = true;

    createProperties();
}

BusOut::~BusOut()
{
    delete volumeValue;
    delete volumeListener;
}

void BusOut::createProperties() {
    volumeValue = new juce::Value();
    volumeListener = new ValueListener(*volumeValue, this);
}

void BusOut::configurePins() {
    // Bewusst leer: BusOut hat keine Patch-Pins. Tracks senden via
    // Track::Send / Mixer-Send-Logik direkt in den internen mixBuffer.
}

void BusOut::paint(juce::Graphics& g) {
    Module::paint(g);
    // Wir teilen uns das Aux-Bild — semantisch das gleiche Symbol
    // (Stereo-Bus), spart eine eigene PNG-Ressource im BinaryData.
    g.drawImageAt(ImageCache::getFromMemory(BinaryData::auxmodule_png, BinaryData::auxmodule_pngSize), 25, 40);
}

void BusOut::process() {
    // Sends werden im Audio-Callback summiert — der Modul-Graph hat hier
    // nichts zu tun.
}

void BusOut::clearMixBuffer(int numSamples) {
    if (mixBuffer.getNumSamples() != numSamples || mixBuffer.getNumChannels() != 2) {
        // Allow shrinking (keepExisting=false) and reallocate only when shape changes —
        // im stationaeren Zustand (Buffersize stabil) ist das ein No-Op.
        mixBuffer.setSize(2, numSamples, false, true, true);
    }
    mixBuffer.clear();
}

void BusOut::setChannelIndex(int index) {
    this->channelIndex = index;
}

int BusOut::getChannelIndex() {
    return channelIndex;
}

juce::Array<juce::PropertyComponent*>& BusOut::getProperties() {
    properties = juce::Array<juce::PropertyComponent*>();
    volumeProp = new juce::SliderPropertyComponent(*volumeValue, "Volume", 0, 1, 0.01, 1.0, true);
    properties.add(volumeProp);
    return properties;
}
