#include "MidiRecorderModule.h"
#include "AudioManager.h"

MidiRecorderModule::MidiRecorderModule()
{
    setSize(120, 140);
    createProperties();
    setName("MIDI Recorder");
    prefab = true;
    editable = false;
    this->deviceManager = AudioManager::getInstance()->getDeviceManager();
}

MidiRecorderModule::~MidiRecorderModule()
{
    properties.clear();
    delete tempoValue;
    delete tempoListener;
    delete recordingStateValue;
    delete recordingStateListener;
    delete playingStateValue;
    delete playingStateListener;
    delete midiInPin;
    delete midiOutPin;
}

void MidiRecorderModule::configurePins()
{
    midiInPin = new Pin(Pin::Type::EVENT);
    midiInPin->direction = Pin::Direction::IN;
    midiInPin->setName("MIDI IN");
    addPin(Pin::Direction::IN, midiInPin);
    midiInPin->addEventListener(this);

    midiOutPin = new Pin(Pin::Type::EVENT);
    midiOutPin->direction = Pin::Direction::OUT;
    midiOutPin->setName("MIDI OUT");
    addPin(Pin::Direction::OUT, midiOutPin);
}

void MidiRecorderModule::paint(juce::Graphics& g)
{
    Module::paint(g);

    g.setColour(juce::Colours::white);
    g.setFont(14.0f);

    if (isRecording()) {
        g.drawText("Recording...", getLocalBounds(), juce::Justification::centred);
        g.drawText(juce::String(buffer.getNumEvents()) + " events", getLocalBounds(), juce::Justification::centredBottom);
    }
    else if (isPlaying()) {
        g.drawText("Playing...", getLocalBounds(), juce::Justification::centred);
    }
    else {
        g.drawText("Idle", getLocalBounds(), juce::Justification::centred);
    }
}

void MidiRecorderModule::timerCallback()
{
    double currentTime = (juce::Time::getMillisecondCounterHiRes() - startTime);

    juce::MidiBuffer::Iterator it(buffer);
    juce::MidiMessage message;
    int samplePosition;

    // Skip processed events
    for (int i = 0; i < playbackIndex; ++i) {
        it.getNextEvent(message, samplePosition);
    }

    // Process remaining events
    while (it.getNextEvent(message, samplePosition))
    {
        double eventTime = message.getTimeStamp();

        if (eventTime <= currentTime)
        {
            // Send the MIDI event
            Event* e = new Event();
            e->setMessage(message);
            e->setType(Event::Type::GATE);
            e->setNote(message.getNoteNumber());
            e->setValue(message.isNoteOn() ? message.getVelocity() : 0);
            midiOutPin->sendEvent(e);            
            playbackIndex++;
        }
        else
        {
            break; // Stop if the event is in the future
        }
    }

    // If all events are processed, stop playback
    if (playbackIndex >= buffer.getNumEvents())
    {
        stopPlaying();
    }
}

void MidiRecorderModule::startRecording()
{
    if (deviceManager) {
        buffer.clear();
        startTime = juce::Time::getMillisecondCounterHiRes();

        if (recordingStateValue == nullptr) {
            recordingStateValue = new Value();
        }
        recordingStateValue->setValue(true);
        repaint();
    }
}

void MidiRecorderModule::stopRecording()
{
    if (recordingStateValue == nullptr) {
        recordingStateValue = new Value();
    }
    recordingStateValue->setValue(false);
    repaint();
}

void MidiRecorderModule::startPlaying()
{
    if (deviceManager) {
        if (playingStateValue == nullptr) {
            playingStateValue = new Value();
        }
        playingStateValue->setValue(true);
        startTime = juce::Time::getMillisecondCounterHiRes();
        playbackIndex = 0; // Start from the first event
        startTimer(1);     // Start timer with 1ms interval
    }
    repaint();
}

void MidiRecorderModule::stopPlaying()
{
    if (playingStateValue != nullptr) {
        playingStateValue->setValue(false);
    }
    stopTimer();
    repaint();
}

bool MidiRecorderModule::isRecording() const
{
    return recordingStateValue && recordingStateValue->getValue().toString().getIntValue() > 0;
}

bool MidiRecorderModule::isPlaying() const
{
    return playingStateValue && playingStateValue->getValue().toString().getIntValue() > 0;
}

void MidiRecorderModule::setDeviceManager(juce::AudioDeviceManager* manager)
{
    deviceManager = manager;
}

void MidiRecorderModule::setTempo(float newTempo)
{
    if (tempoValue != nullptr) {
        tempoValue->setValue(newTempo);
    }
}

void MidiRecorderModule::createProperties()
{
    tempoValue = new Value();
    tempoValue->setValue(120.0f);
    tempoListener = new TempoListener(*tempoValue, this);

    recordingStateValue = new Value();
    recordingStateListener = new RecordingStateListener(*recordingStateValue, this);

    playingStateValue = new Value();
    playingStateListener = new PlayingStateListener(*playingStateValue, this);
}

void MidiRecorderModule::eventReceived(Event* e)
{
    // Handle incoming events if needed
}

void MidiRecorderModule::handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message)
{
    if (message.isNoteOn() || message.isNoteOff()) {
        Event* e = new Event();
        e->setMessage(message);
        e->setType(Event::Type::GATE);
        e->setNote(message.getNoteNumber());
        e->setValue(message.isNoteOn() ? message.getVelocity() : 0);
        midiOutPin->sendEvent(e);

        if (isRecording()) {
            buffer.addEvent(message, juce::Time::getMillisecondCounterHiRes() - startTime);
        }
    }
    repaint();
}

juce::Array<juce::PropertyComponent*>& MidiRecorderModule::getProperties()
{
    properties = juce::Array<PropertyComponent*>();

    tempoProperty = new juce::SliderPropertyComponent(*tempoValue, "Tempo", 1, 240, 0.1, 1.0, true);
    recordingStateProperty = new juce::BooleanPropertyComponent(*recordingStateValue, "Recording", "Recording");
    playingStateProperty = new juce::BooleanPropertyComponent(*playingStateValue, "Playing", "Playing");

    properties.add(tempoProperty);
    properties.add(recordingStateProperty);
    properties.add(playingStateProperty);
    return properties;
}
