/*
 ==============================================================================

 MidiGate.h
 Created: 4 Apr 2018 3:48:23pm
 Author:  Matthias Pueski

 ==============================================================================
 */

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"
#include "VolumeAdjustable.h"
#include "Tracks/Track.h"
#include "Tracks/TrackNavigator.h"

 //==============================================================================
 /*
  */
class TrackIn : public Module, public VolumeAdjustable, public ChangeListener
{
public:
    TrackIn(TrackNavigator* navigator, double sampleRate, int buffersize);
    ~TrackIn();

    void paint(juce::Graphics& g) override;
    void setDeviceManager(juce::AudioDeviceManager* manager);
    virtual void process() override;

    virtual void configurePins() override;

    virtual juce::String getCategory() override {
        return "Input / Output";
    }

    // this is needed to associate the channel with the mixer
    int getChannelIndex();
    void setChannelIndex(int index);

    void setSamplePosition(int samples);

    Track* getTrack();
    void setTrack(Track* t);

    struct ValueListener : juce::Value::Listener
    {
        ValueListener(juce::Value& v, TrackIn* a) : a(a), value(v) { value.addListener(this); }
        ~ValueListener() {}  // no need to remove the listener

        void valueChanged(juce::Value& value) override {
            a->setVolume(value.toString().getFloatValue());
        }
        TrackIn* a;
        juce::Value value;
    };

    struct TrackListener : juce::Value::Listener
    {
        TrackListener(juce::Value& v, TrackIn* a) : a(a), value(v) { value.addListener(this); }
        ~TrackListener() {}  // no need to remove the listener

        void valueChanged(juce::Value& value) override {
            for (int i = 0; i < a->navigator->getTracks().size(); i++) {
                if (a->navigator->getTracks().at(i)->getName() == value.toString()) {
                    a->setTrack(a->navigator->getTracks().at(i));
                    break;
                }
            }
        }
        TrackIn* a;
        juce::Value value;
    };

    virtual void setVolume(float volume) override {
        this->volumeValue->setValue(volume);
        this->volume = volume;
        VolumeAdjustable::sendChangeMessage();
    }

    virtual juce::Array<juce::PropertyComponent*>& getProperties() override;
    virtual void createProperties() override;

    void changeListenerCallback(ChangeBroadcaster* source);

private:
    juce::Value* volumeValue;
    juce::PropertyComponent* volumeProp;
    ValueListener* volumeListener;

    juce::Value* trackValue;
    juce::PropertyComponent* trackProp;
    TrackListener* trackListener;

    Track* track;
    TrackNavigator* navigator;
    int numSamples = 0;
    int offset = 0;
    juce::AudioDeviceManager* deviceManager;
    int channelIndex;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackIn)
};

