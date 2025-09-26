/*
  ==============================================================================

    MidiOut.h
    Created: 4 Apr 2018 3:48:23pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"

//==============================================================================
/*
*/
class MidiOut : public Module
{
public:
    MidiOut();
    ~MidiOut();
    
    void paint(juce::Graphics& g) override;
    void eventReceived(Event* e) override;
    void configurePins() override;
    
    juce::String getCategory() override {
        return "MIDI";
    }
    
    // MIDI device management
    void updateAvailableDevices();
    void setSelectedMidiOutput(int deviceIndex);
    void setMidiChannel(int channel);
    
    // Property management
    juce::Array<juce::PropertyComponent*>& getProperties() override;
    void createProperties() override;

private:
    // MIDI output management
    std::unique_ptr<juce::MidiOutput> currentMidiOutput;
    juce::StringArray availableOutputs;
    int selectedMidiOutputIndex = 0;
    int midiChannel = 1;
    juce::AudioDeviceManager* deviceManager;
    
    // Activity indicator
    bool isActive = false;
    
    // Event handlers
    void sendMidiMessage(const juce::MidiMessage& message);
    void handleGateEvent(Event* e);
    void handleNoteEvent(Event* e);
    void handleControllerEvent(Event* e);
    void handlePitchEvent(Event* e);
    void handleClockEvent(Event* e);
    
    // Property components and values
    juce::Value* deviceValue = nullptr;
    juce::Value* channelValue = nullptr;
    juce::ChoicePropertyComponent* deviceProp = nullptr;
    juce::SliderPropertyComponent* channelProp = nullptr;
    
    // Value change listeners
    class DeviceListener : public juce::Value::Listener
    {
    public:
        DeviceListener(juce::Value& v, MidiOut* owner) : value(v), midiOut(owner)
        {
            value.addListener(this);
        }
        
        ~DeviceListener()
        {
            value.removeListener(this);
        }
        
        void valueChanged(juce::Value& value) override
        {
            if (midiOut != nullptr)
            {
                midiOut->setSelectedMidiOutput((int)value.getValue());
            }
        }
        
    private:
        juce::Value& value;
        MidiOut* midiOut;
    };
    
    class ChannelListener : public juce::Value::Listener
    {
    public:
        ChannelListener(juce::Value& v, MidiOut* owner) : value(v), midiOut(owner)
        {
            value.addListener(this);
        }
        
        ~ChannelListener()
        {
            value.removeListener(this);
        }
        
        void valueChanged(juce::Value& value) override
        {
            if (midiOut != nullptr)
            {
                midiOut->setMidiChannel((int)value.getValue());
            }
        }
        
    private:
        juce::Value& value;
        MidiOut* midiOut;
    };
    
    DeviceListener* deviceListener = nullptr;
    ChannelListener* channelListener = nullptr;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiOut)
};
