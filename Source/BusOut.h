/*
 ==============================================================================

 BusOut.h

 Wie AuxOut, aber ohne Input-Pins: ein BusOut nimmt keine Patch-Kabel auf.
 Statt dessen halten Tracks eine Send-Liste (Track::Send), und der Audio-
 Callback summiert pro Send das post-fader Tracksignal direkt in den
 internen Stereo-Mix-Buffer dieses BusOut. Im selben Block wird der Bus
 dann genauso wie ein AuxOut auf den Master gemischt (mit eigenem
 Volume / Pan / Mute / Solo der zugeordneten Mixer::Channel).

 Persistenz analog zu AuxOut — BusOut lebt als ganz normales Modul im
 Modul-Graphen (mit stabilem Module::getIndex()), nur eben ohne Inputs.
 Der Index ist die stabile Referenz, die Track::Send beim Save in der
 tracks.xml speichert.

 ==============================================================================
 */

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"
#include "VolumeAdjustable.h"

class BusOut : public Module, public VolumeAdjustable
{
public:
    BusOut();
    ~BusOut();

    void paint(juce::Graphics& g) override;
    virtual void process() override;
    virtual void configurePins() override;

    virtual juce::String getCategory() override {
        return "Input / Output";
    }

    int getChannelIndex();
    void setChannelIndex(int index);

    // Vor dem Aufsummieren der Sends einmal pro Audio-Block leeren — sonst
    // akkumuliert der Bus block-für-block und beschleunigt zur Selbst-
    // Übersteuerung. Aufrufer: MainComponent::getNextAudioBlock und
    // BounceThread::run, immer NACH processModule (damit Track-Audio
    // fertig vorliegt) und VOR dem Send-Mix-Loop.
    void clearMixBuffer(int numSamples);
    // Liefert den internen Stereo-Buffer, in den Sends hineingemischt werden
    // und aus dem die Master-Summation liest. Größe wird in clearMixBuffer()
    // sichergestellt.
    juce::AudioSampleBuffer* getMixBuffer() { return &mixBuffer; }

    struct ValueListener : juce::Value::Listener
    {
        ValueListener (juce::Value& v, BusOut* b) : b(b), value(v) { value.addListener(this); }
        ~ValueListener() {}

        void valueChanged (juce::Value& value) override {
            b->setVolume(value.toString().getFloatValue());
        }
        BusOut* b;
        juce::Value value;
    };

    virtual void setVolume(float volume) override {
        this->volumeValue->setValue(volume);
        this->volume = volume;
        VolumeAdjustable::sendChangeMessage();
    }

    virtual juce::Array<juce::PropertyComponent*>& getProperties() override;
    virtual void createProperties() override;

private:
    juce::Value* volumeValue;
    juce::PropertyComponent* volumeProp;
    ValueListener* volumeListener;

    int channelIndex = 0;

    // Internes Stereo-Mix-Ziel für alle Sends, die auf diesen Bus zeigen.
    // Wird im Audio-Thread beschrieben — keine Realloc nach dem ersten
    // clearMixBuffer() im Steady-State (setSize mit avoidReallocating=true).
    juce::AudioSampleBuffer mixBuffer { 2, 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BusOut)
};
