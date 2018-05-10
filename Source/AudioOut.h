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

//==============================================================================
/*
*/
class AudioOut    : public Module, public VolumeAdjustable
{
public:
    AudioOut();
    ~AudioOut();
   
    void paint (juce::Graphics& g) override;
    void setDeviceManager(juce::AudioDeviceManager* manager);
    virtual void process() override;

    // this is needed to associate the channel with the mixer
    int getChannelIndex();
    void setChannelIndex(int index);
    
    virtual void configurePins() override;
    
    virtual juce::String getCategory() override {
        return "Input / Output";
    }
    
    struct ValueListener : juce::Value::Listener
    {
        ValueListener (juce::Value& v, AudioOut* a) : a(a),  value (v) { value.addListener (this); }
        ~ValueListener()  {}  // no need to remove the listener
        
        void valueChanged (juce::Value& value) override {
            a->setVolume(value.toString().getFloatValue());
        }
        AudioOut* a;
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
    
    juce::AudioDeviceManager* deviceManager;
    int channelIndex;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioOut)
};
