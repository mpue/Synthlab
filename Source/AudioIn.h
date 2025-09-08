/*
 ==============================================================================

 AudioIn.h
 Created: 4 Apr 2018 3:48:23pm
 Author:  Matthias Pueski
 Modified: Erweitert um Audioeingang-Auswahl

 ==============================================================================
 */
#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"
#include "VolumeAdjustable.h"
 //==============================================================================
 /*
  */
class AudioIn : public Module, public VolumeAdjustable
{
public:
    AudioIn();
    ~AudioIn();

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

    // Neue Methoden für Audioeingang-Auswahl
    void setSelectedAudioInput(int inputIndex);
    int getSelectedAudioInput() const;
    juce::StringArray getAvailableAudioInputs() const;
    void updateAudioInputList();

    struct ValueListener : juce::Value::Listener
    {
        ValueListener(juce::Value& v, AudioIn* a) : a(a), value(v) { value.addListener(this); }
        ~ValueListener() {}  // no need to remove the listener

        void valueChanged(juce::Value& value) override {
            a->setVolume(value.toString().getFloatValue());
        }
        AudioIn* a;
        juce::Value value;
    };

    struct AudioInputListener : juce::Value::Listener
    {
        AudioInputListener(juce::Value& v, AudioIn* a) : a(a), value(v) { value.addListener(this); }
        ~AudioInputListener() {}

        void valueChanged(juce::Value& value) override {
            a->setSelectedAudioInput(value.toString().getIntValue());
        }
        AudioIn* a;
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
    juce::Value* audioInputValue;
    juce::PropertyComponent* volumeProp;
    juce::PropertyComponent* audioInputProp;
    ValueListener* volumeListener;
    AudioInputListener* audioInputListener;

    juce::AudioDeviceManager* deviceManager;
    int channelIndex;
    int selectedAudioInputIndex;
    juce::StringArray availableInputs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioIn)
};