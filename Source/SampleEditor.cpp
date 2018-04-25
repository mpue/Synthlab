
#include "AudioManager.h"
#include "SampleEditor.h"
#include "push2/JuceToPush2DisplayBridge.h"



//==============================================================================
SampleEditor::SampleEditor (int buffersize, float sampleRate, AudioFormatManager* manager, SamplerModule* module)
{

    this->sampleModule = module;

    addAndMakeVisible (midiKeyboard = new MidiKeyboardComponent (state,orientation));
    addAndMakeVisible (component = new AudioThumbnailComponent(buffersize, sampleRate));
    addAndMakeVisible(propertiesPanel = new SamplePropertiesPanel());
    
    setSize (500, 400);
    
    if (sampleModule->getBuffer() != nullptr) {
        component->setSampleBuffer(sampleModule->getBuffer());
    }
    state.addListener(this);
    
    AudioDeviceManager* deviceManager = AudioManager::getInstance()->getDeviceManager();
    
    for (int i = 0; i < MidiInput::getDevices().size();i++) {
        if (deviceManager->isMidiInputEnabled(MidiInput::getDevices().getReference(i))) {
            deviceManager->addMidiInputCallback(MidiInput::getDevices().getReference(i),this);
        }
    }
    
    propertiesPanel->setModule(module);
    propertiesPanel->setThumbnail(component);
    
}

SampleEditor::~SampleEditor()
{
    state.removeListener(this);
    midiKeyboard = nullptr;
    delete component;
    propertiesPanel = nullptr;

    AudioDeviceManager* deviceManager = AudioManager::getInstance()->getDeviceManager();
    
    for (int i = 0; i < MidiInput::getDevices().size();i++) {
        if (deviceManager->isMidiInputEnabled(MidiInput::getDevices().getReference(i))) {
            deviceManager->removeMidiInputCallback(MidiInput::getDevices().getReference(i),this);
        }
    }
    
}

void SampleEditor::paint (Graphics& g)
{

    g.fillAll (Colours::darkgrey);

}

void SampleEditor::resized()
{
    
    component->setBounds (0, 0, proportionOfWidth (1.0000f), proportionOfHeight(0.4f));
    propertiesPanel->setBounds (0, proportionOfHeight(0.4f), proportionOfWidth (1.0000f), proportionOfHeight(0.4f));
    midiKeyboard->setBounds (0, proportionOfHeight(0.8), proportionOfWidth (1.0000f), proportionOfHeight(0.2f));
}

void SampleEditor::handleIncomingMidiMessage (MidiInput* source, const MidiMessage& message) {
    if (message.isNoteOn()) {
        handleNoteOn(&state, message.getChannel(), message.getNoteNumber(), message.getVelocity());
    }
    else if(message.isNoteOff()) {
        handleNoteOff(&state, message.getChannel(), message.getNoteNumber(), message.getVelocity());
    }
}

void SampleEditor::handleNoteOn(juce::MidiKeyboardState *source, int midiChannel, int midiNoteNumber, float velocity) {
    sampleModule->selectSample(midiNoteNumber);
    propertiesPanel->updateValues();
    component->setSampleBuffer(sampleModule->getBuffer());

}

void SampleEditor::handleNoteOff(juce::MidiKeyboardState *source, int midiChannel, int midiNoteNumber, float velocity) {
    
}

