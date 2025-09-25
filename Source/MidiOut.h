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
Enhanced MIDI Output Module with device selection and complete event support
*/
class MidiOut : public Module
{
public:
    MidiOut();
    ~MidiOut();
    
    void paint (juce::Graphics& g) override;
    void eventReceived(Event* e) override;
    void configurePins() override;
    
    // Properties support
    juce::Array<juce::PropertyComponent*>& getProperties() override;
    void createProperties();
    
    // MIDI device management
    void updateAvailableDevices();
    void setSelectedMidiOutput(int deviceIndex);
    int getSelectedMidiOutput() const { return selectedMidiOutputIndex; }
    juce::StringArray getAvailableMidiOutputs() const { return availableOutputs; }
    
    // MIDI channel management
    void setMidiChannel(int channel);
    int getMidiChannel() const { return midiChannel; }
    
    virtual juce::String getCategory() override {
        return "MIDI";
    }
    
private:
    // MIDI message sending
    void sendMidiMessage(const juce::MidiMessage& message);
    
    // Event type handlers
    void handleGateEvent(Event* e);
    void handleNoteEvent(Event* e);
    void handleControllerEvent(Event* e);
    void handlePitchEvent(Event* e);
    void handleClockEvent(Event* e);
    
    // Device management
    juce::AudioDeviceManager* deviceManager;
    juce::StringArray availableOutputs;
    int selectedMidiOutputIndex = 0;
    std::unique_ptr<juce::MidiOutput> currentMidiOutput;
    
    // MIDI state
    int midiChannel = 1;
    bool isActive = false;
    
    // Properties
    juce::Array<juce::PropertyComponent*> properties;
    
    // Device selection property
    juce::Value* deviceValue = nullptr;
    struct DeviceListener : juce::Value::Listener {
        DeviceListener(juce::Value& v, MidiOut* m) : owner(m), value(v) {
            value.addListener(this);
        }
        ~DeviceListener() = default;
        
        void valueChanged(juce::Value& value) override {
            owner->setSelectedMidiOutput(value.toString().getIntValue());
        }
        
        MidiOut* owner;
        juce::Value value;
    };
    DeviceListener* deviceListener = nullptr;
    juce::PropertyComponent* deviceProp = nullptr;
    
    // Channel selection property
    juce::Value* channelValue = nullptr;
    struct ChannelListener : juce::Value::Listener {
        ChannelListener(juce::Value& v, MidiOut* m) : owner(m), value(v) {
            value.addListener(this);
        }
        ~ChannelListener() = default;
        
        void valueChanged(juce::Value& value) override {
            owner->setMidiChannel(value.toString().getIntValue());
        }
        
        MidiOut* owner;
        juce::Value value;
    };
    ChannelListener* channelListener = nullptr;
    juce::PropertyComponent* channelProp = nullptr;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiOut)
};
