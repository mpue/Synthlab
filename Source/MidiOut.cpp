/*
  ==============================================================================

    MidiOut.cpp
    Created: 4 Apr 2018 3:48:23pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "MidiOut.h"
#include "AudioManager.h"

using juce::ImageCache;
using juce::MidiMessage;
using juce::uint8;
using juce::Logger;

//==============================================================================
MidiOut::MidiOut()
{
    setSize(120, 140);
    
    editable = false;
    prefab = true;
    
    this->deviceManager = AudioManager::getInstance()->getDeviceManager();
    
    // Update available MIDI outputs
    updateAvailableDevices();
    
    // Try to open the default MIDI output
    if (!availableOutputs.isEmpty()) {
        setSelectedMidiOutput(0);
    }
    
    createProperties();
}

MidiOut::~MidiOut()
{
    currentMidiOutput.reset();   
}

void MidiOut::configurePins() {
    // Gate/Note events input
    Pin* gatePin = new Pin(Pin::Type::EVENT);
    gatePin->direction = Pin::Direction::IN;
    gatePin->listeners.push_back(this);
    gatePin->setName("G");
    
    // Note input
    Pin* notePin = new Pin(Pin::Type::EVENT);
    notePin->direction = Pin::Direction::IN;
    notePin->listeners.push_back(this);
    notePin->setName("N");
    
    // Controller input
    Pin* ctrlPin = new Pin(Pin::Type::EVENT);
    ctrlPin->direction = Pin::Direction::IN;
    ctrlPin->listeners.push_back(this);
    ctrlPin->setName("C");
    
    // Pitch bend input
    Pin* pitchPin = new Pin(Pin::Type::EVENT);
    pitchPin->direction = Pin::Direction::IN;
    pitchPin->listeners.push_back(this);
    pitchPin->setName("P");

    addPin(Pin::Direction::IN, gatePin);
    addPin(Pin::Direction::IN, notePin);
    addPin(Pin::Direction::IN, ctrlPin);
    addPin(Pin::Direction::IN, pitchPin);
}

void MidiOut::paint(juce::Graphics &g) {
    Module::paint(g);
    g.drawImageAt(ImageCache::getFromMemory(BinaryData::midiout_png, BinaryData::midiout_pngSize), 25, 40);
    
    // Draw activity LED
    const int ledSize = 10;
    const int x = getWidth() - ledSize - 4;
    const int y = getHeight() - ledSize - 4;
    
    g.setColour(isActive ? juce::Colours::green : juce::Colours::darkgrey);
    g.fillEllipse((float)x, (float)y, (float)ledSize, (float)ledSize);
    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.drawEllipse((float)x, (float)y, (float)ledSize, (float)ledSize, 1.0f);
    
    // Draw device name if available
    if (selectedMidiOutputIndex >= 0 && selectedMidiOutputIndex < availableOutputs.size()) {
        g.setColour(juce::Colours::white);
        g.setFont(10.0f);
        juce::String deviceName = availableOutputs[selectedMidiOutputIndex];
        if (deviceName.length() > 12) {
            deviceName = deviceName.substring(0, 10) + "...";
        }
        g.drawText(deviceName, 5, 110, 110, 15, juce::Justification::centred);
    }
}

void MidiOut::sendMidiMessage(const juce::MidiMessage& message) {
    if (currentMidiOutput != nullptr) {
        currentMidiOutput->sendMessageNow(message);
        isActive = true;
        
        // Reset activity indicator after a short delay
        juce::Timer::callAfterDelay(50, [this]() {
            isActive = false;
            repaint();
        });
        Logger::getCurrentLogger()->writeToLog("MIDI Out: " + message.getDescription());
    }
}

void MidiOut::handleGateEvent(Event* e) {
    if (e->getValue() > 0) {
        // Note on
        MidiMessage msg = MidiMessage::noteOn(midiChannel, e->getNote(), (uint8)e->getValue());
        sendMidiMessage(msg);
    } else {
        // Note off
        MidiMessage msg = MidiMessage::noteOff(midiChannel, e->getNote(), (uint8)0);
        sendMidiMessage(msg);
    }
}

void MidiOut::handleNoteEvent(Event* e) {
    // For standalone note events, we can send a short note on/off sequence
    MidiMessage noteOn = MidiMessage::noteOn(midiChannel, e->getValue(), (uint8)100);
    MidiMessage noteOff = MidiMessage::noteOff(midiChannel, e->getValue(), (uint8)0);
    
    sendMidiMessage(noteOn);
    
    // Schedule note off after 100ms
    juce::Timer::callAfterDelay(100, [this, noteOff]() {
        sendMidiMessage(noteOff);
    });
}

void MidiOut::handleControllerEvent(Event* e) {
    MidiMessage msg = MidiMessage::controllerEvent(midiChannel, e->getNumber(), e->getValue());
    sendMidiMessage(msg);
}

void MidiOut::handlePitchEvent(Event* e) {
    MidiMessage msg = MidiMessage::pitchWheel(midiChannel, e->getValue());
    sendMidiMessage(msg);
}

void MidiOut::handleClockEvent(Event* e) {
    MidiMessage msg;
    
    switch (e->getType()) {
        case Event::Type::CLOCK:
            msg = MidiMessage::midiClock();
            break;
        case Event::Type::CLOCK_START:
            msg = MidiMessage::midiStart();
            break;
        case Event::Type::CLOCK_STOP:
            msg = MidiMessage::midiStop();
            break;
        default:
            return;
    }
    
    sendMidiMessage(msg);
}

void MidiOut::eventReceived(Event *e) {
    if (e == nullptr) return;
    
    switch (e->getType()) {
        case Event::Type::GATE:
            handleGateEvent(e);
            break;
            
        case Event::Type::NOTE:
            handleNoteEvent(e);
            break;
            
        case Event::Type::CONTROLLER:
            handleControllerEvent(e);
            break;
            
        case Event::Type::PITCH:
            handlePitchEvent(e);
            break;
            
        case Event::Type::CLOCK:
        case Event::Type::CLOCK_START:
        case Event::Type::CLOCK_STOP:
            handleClockEvent(e);
            break;
            
        default:
            Logger::getCurrentLogger()->writeToLog("MidiOut: Unknown event type received");
            break;
    }
}

void MidiOut::updateAvailableDevices() {
    availableOutputs.clear();
    
    // Add "None" option as first entry
    availableOutputs.add("None");
    
    // Get available MIDI devices using the correct JUCE 8 API
    auto devices = juce::MidiOutput::getAvailableDevices();
    
    for (const auto& device : devices) {
        availableOutputs.add(device.name);
    }
    
    // Ensure selected index is valid
    if (selectedMidiOutputIndex >= availableOutputs.size()) {
        selectedMidiOutputIndex = 0;
    }
    
    repaint();
}

void MidiOut::setSelectedMidiOutput(int deviceIndex) {
    if (deviceIndex < 0 || deviceIndex >= availableOutputs.size()) {
        return;
    }
    
    selectedMidiOutputIndex = deviceIndex;
    
    // Close current output
    currentMidiOutput.reset();
    
    if (deviceIndex == 0) {
        // "None" selected
        Logger::getCurrentLogger()->writeToLog("MidiOut: No output device selected");
    } else {
        // Try to open the selected device using the correct JUCE 8 API
        auto devices = juce::MidiOutput::getAvailableDevices();
        
        // Find the device by name (deviceIndex - 1 because of "None" at index 0)
        int realDeviceIndex = deviceIndex - 1;
        if (realDeviceIndex >= 0 && realDeviceIndex < devices.size()) {
            currentMidiOutput = juce::MidiOutput::openDevice(devices[realDeviceIndex].identifier);
            
            if (currentMidiOutput != nullptr) {
                Logger::getCurrentLogger()->writeToLog("MidiOut: Opened device: " + devices[realDeviceIndex].name);
            } else {
                Logger::getCurrentLogger()->writeToLog("MidiOut: Failed to open device: " + devices[realDeviceIndex].name);
            }
        }
    }
    
    if (deviceValue != nullptr) {
        deviceValue->setValue(deviceIndex);
    }
    
    repaint();
}

void MidiOut::setMidiChannel(int channel) {
    if (channel >= 1 && channel <= 16) {
        midiChannel = channel;
        if (channelValue != nullptr) {
            channelValue->setValue(channel);
        }
        Logger::getCurrentLogger()->writeToLog("MidiOut: MIDI channel set to " + juce::String(channel));
    }
}

juce::String MidiOut::getSelectedMidiOutput()
{
    auto devices = juce::MidiOutput::getAvailableDevices();

    for (int i = 0; i < devices.size(); i++) {
        if (i + 1 == selectedMidiOutputIndex) { // +1 because of "None" at index 0
            return devices[i].name;
        }
	}
        
}

void MidiOut::setSelectedMidiOutput(const juce::String& deviceName)
{
    auto devices = juce::MidiOutput::getAvailableDevices();
    for (int i = 0; i < devices.size(); i++) {
        if (devices[i].name == deviceName) {
            setSelectedMidiOutput(i + 1); // +1 because of "None" at index 0
            return;
        }
    }
    // If device not found, select "None"
	setSelectedMidiOutput(0);
}

juce::Array<juce::PropertyComponent*>& MidiOut::getProperties() {
    properties.clear();
    
    // MIDI Device selection
    if (deviceProp != nullptr) {
        juce::Array<juce::var> deviceValues;
        juce::StringArray deviceNames;
        
        for (int i = 0; i < availableOutputs.size(); i++) {
            deviceValues.add(i);
            deviceNames.add(availableOutputs[i]);
        }
        
        deviceProp = new juce::ChoicePropertyComponent(*deviceValue, "MIDI Device", deviceNames, deviceValues);
        properties.add(deviceProp);
    }
    
    // MIDI Channel selection
    if (channelProp != nullptr) {
        channelProp = new juce::SliderPropertyComponent(*channelValue, "MIDI Channel", 1, 16, 1);
        properties.add(channelProp);
    }
    
    return properties;
}

void MidiOut::createProperties() {
    // Device selection
    deviceValue = std::make_unique<juce::Value>(selectedMidiOutputIndex);
    deviceListener = std::make_unique<DeviceListener>(*deviceValue, this);
    
    // Channel selection
    channelValue = std::make_unique<juce::Value>(midiChannel);
    channelListener = std::make_unique<ChannelListener>(*channelValue, this);
    
    // Create property components
    juce::Array<juce::var> deviceValues;
    juce::StringArray deviceNames;
    
    for (int i = 0; i < availableOutputs.size(); i++) {
        deviceValues.add(i);
        deviceNames.add(availableOutputs[i]);
    }
    
    deviceProp = new juce::ChoicePropertyComponent(*deviceValue, "MIDI Device", deviceNames, deviceValues);
    channelProp = new juce::SliderPropertyComponent(*channelValue, "MIDI Channel", 1, 16, 1);
}
