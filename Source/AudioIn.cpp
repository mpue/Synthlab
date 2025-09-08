/*
 ==============================================================================

 AudioIn.cpp
 Created: 4 Apr 2018 3:48:23pm
 Author:  Matthias Pueski
 Modified: Erweitert um Audioeingang-Auswahl

 ==============================================================================
 */
#include "../JuceLibraryCode/JuceHeader.h"
#include "AudioIn.h"
#include "Connection.h"
#include "AudioManager.h"

using juce::Justification;
using juce::ImageCache;
//==============================================================================
AudioIn::AudioIn() : selectedAudioInputIndex(0), deviceManager(nullptr)
{
    setSize(120, 140);
    nameLabel->setJustificationType(Justification::left);
    nameLabel->setTopLeftPosition(18, 2);
    setName("Audio in");

    editable = false;
    prefab = true;
	setDeviceManager(AudioManager::getInstance()->getDeviceManager());  

    createProperties();

}

AudioIn::~AudioIn()
{
    delete volumeValue;
    delete volumeListener;
    delete audioInputValue;
    delete audioInputListener;
}

void AudioIn::createProperties() {
    volumeValue = new juce::Value();
    volumeListener = new ValueListener(*volumeValue, this);

    audioInputValue = new juce::Value();
    audioInputListener = new AudioInputListener(*audioInputValue, this);
    audioInputValue->setValue(0); // Standard: Erster verfügbarer Eingang
}

void AudioIn::configurePins() {
    Pin* p1 = new Pin(Pin::Type::AUDIO);
    p1->direction = Pin::Direction::IN;
    p1->setName("In");

    //Pin* p2 = new Pin(Pin::Type::AUDIO);
    //p2->direction = Pin::Direction::IN;
    //p2->setName("R");

    addPin(Pin::Direction::OUT, p1);
    // addPin(Pin::Direction::OUT, p2);
}

void AudioIn::paint(juce::Graphics& g) {
    Module::paint(g);
    g.drawImageAt(ImageCache::getFromMemory(BinaryData::micro_png, BinaryData::micro_pngSize), 25, 40);

    // Anzeige des gewählten Audioeingangs
    if (!availableInputs.isEmpty() && selectedAudioInputIndex < availableInputs.size()) {
        g.setColour(juce::Colours::white);
        g.setFont(10.0f);
        juce::String inputName = availableInputs[selectedAudioInputIndex];
        if (inputName.length() > 12) {
            inputName = inputName.substring(0, 10) + "...";
        }
        g.drawText(inputName, 5, 110, 110, 15, Justification::centred);
    }
	
}

void AudioIn::process() {
	// no pocessing needed, audio comes directly from the device
}

void AudioIn::setDeviceManager(juce::AudioDeviceManager* manager) {
    this->deviceManager = manager;
    updateAudioInputList();
}

void AudioIn::setChannelIndex(int index) {
    this->channelIndex = index;
}

int AudioIn::getChannelIndex() {
    return channelIndex;
}

void AudioIn::setSelectedAudioInput(int inputIndex) {
    if (inputIndex >= 0 && inputIndex < availableInputs.size()) {
        selectedAudioInputIndex = inputIndex;
        audioInputValue->setValue(inputIndex);
        repaint(); // UI aktualisieren

        // Hier könnte eine Callback-Funktion aufgerufen werden,
        // um das Audio-System über den Wechsel zu informieren
    }
}

int AudioIn::getSelectedAudioInput() const {
    return selectedAudioInputIndex;
}

juce::StringArray AudioIn::getAvailableAudioInputs() const {
    return availableInputs;
}

void AudioIn::updateAudioInputList() {
    availableInputs.clear();

    if (deviceManager != nullptr) {
        auto* currentDevice = deviceManager->getCurrentAudioDevice();
        if (currentDevice != nullptr) {
            availableInputs = currentDevice->getInputChannelNames();
        }
    }

    // Fallback: Wenn keine Eingänge verfügbar sind
    if (availableInputs.isEmpty()) {
        availableInputs.add("No inputs available");
    }

    // Stelle sicher, dass der gewählte Index gültig ist
    if (selectedAudioInputIndex >= availableInputs.size()) {
        selectedAudioInputIndex = 0;
        audioInputValue->setValue(0);
    }

    repaint();
}

juce::Array<juce::PropertyComponent*>& AudioIn::getProperties() {
    properties = juce::Array<juce::PropertyComponent*>();

    // Volume-Eigenschaft
    volumeProp = new juce::SliderPropertyComponent(*volumeValue, "Volume", 0, 1, 0.01, 1.0, true);
    properties.add(volumeProp);
  
    Array<var> values = Array<var>();
    for (int i = 0; i < availableInputs.size(); i++) {
        values.add(i);
    }

    // Audio Input Auswahl-Eigenschaft
    if (!availableInputs.isEmpty()) {
        audioInputProp = new juce::ChoicePropertyComponent(*audioInputValue, "Audio Input", availableInputs, values);
        properties.add(audioInputProp);
    }

    return properties;
}