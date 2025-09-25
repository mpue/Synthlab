#include "MidiRecorderModule.h"
#include "AudioManager.h"
#include "Connection.h"

MidiRecorderModule::MidiRecorderModule()
{
    setSize(250, 140);
    nameLabel->setJustificationType(juce::Justification::left);
    nameLabel->setTopLeftPosition(2, 2);
    
    createProperties();
    prefab = true;
    editable = false;
    this->deviceManager = AudioManager::getInstance()->getDeviceManager();
}

MidiRecorderModule::~MidiRecorderModule()
{
    stopTimer();
    properties.clear();
    
    delete tempoValue;
    delete tempoListener;
    delete recordingStateValue;
    delete recordingStateListener;
    delete playingStateValue;
    delete playingStateListener;
    
    if (currentSequence) {
        delete currentSequence;
    }
}

void MidiRecorderModule::configurePins()
{
    // MIDI input pin
    midiInPin = new Pin(Pin::Type::EVENT);
    midiInPin->direction = Pin::Direction::IN;
    midiInPin->setName("IN");
    midiInPin->setDescription("MIDI events input for recording");
    midiInPin->listeners.push_back(this);
    addPin(Pin::Direction::IN, midiInPin);

    // MIDI output pin
    midiOutPin = new Pin(Pin::Type::EVENT);
    midiOutPin->direction = Pin::Direction::OUT;
    midiOutPin->setName("OUT");
    midiOutPin->setDescription("MIDI events output during playback");
    addPin(Pin::Direction::OUT, midiOutPin);

    // Trigger input pin
    triggerPin = new Pin(Pin::Type::EVENT);
    triggerPin->direction = Pin::Direction::IN;
    triggerPin->setName("TRIG");
    triggerPin->setDescription("Trigger input to start sequence playback");
    triggerPin->listeners.push_back(this);
    addPin(Pin::Direction::IN, triggerPin);
}

void MidiRecorderModule::paint(juce::Graphics& g)
{
    Module::paint(g);

    g.setColour(juce::Colours::white);
    g.setFont(12.0f);

    // Status display
    if (isRecording()) {
        g.setColour(juce::Colours::red);
        g.fillEllipse(10, 10, 10, 10);
        g.setColour(juce::Colours::white);
        g.drawText("REC", 25, 8, 50, 14, juce::Justification::left);
        g.drawText(juce::String(buffer.getNumEvents()) + " events", 10, 25, 100, 14, juce::Justification::left);
    }
    else if (isPlaying()) {
        g.setColour(juce::Colours::green);
        g.fillEllipse(10, 10, 10, 10);
        g.setColour(juce::Colours::white);
        g.drawText("PLAY", 25, 8, 50, 14, juce::Justification::left);
    }
    else {
        g.setColour(juce::Colours::grey);
        g.fillEllipse(10, 10, 10, 10);
        g.setColour(juce::Colours::white);
        g.drawText("STOP", 25, 8, 50, 14, juce::Justification::left);
    }

    // File name display
    g.setFont(10.0f);
    juce::String fileName = getCurrentFileName();
    if (fileName.isNotEmpty()) {
        if (hasUnsavedChanges) {
            fileName += " *";
        }
        g.drawText(fileName, 10, getHeight() - 30, getWidth() - 20, 12, juce::Justification::left);
    }

    // Tempo display
    g.drawText("BPM: " + juce::String(tempo, 1), 10, getHeight() - 18, getWidth() - 20, 12, juce::Justification::left);

    // Controls hint
    g.setFont(8.0f);
    g.setColour(juce::Colours::lightgrey);
    g.drawText("Right-click for menu", getWidth() - 120, getHeight() - 12, 115, 10, juce::Justification::right);
}

void MidiRecorderModule::timerCallback()
{
    if (!isPlaying()) return;

    double currentTime = juce::Time::getMillisecondCounterHiRes() * 0.001 - sequenceStartTime;

    // Handle MIDI file playback
    if (currentSequence && currentEventIndex < currentSequence->getNumEvents()) {
        const juce::MidiMessageSequence::MidiEventHolder* event = currentSequence->getEventPointer(currentEventIndex);
        
        if (event && event->message.getTimeStamp() <= currentTime) {
            // Send the MIDI event
            Event* e = new Event("midi", Event::Type::GATE);
            e->setMessage(event->message);
            e->setNote(event->message.getNoteNumber());
            e->setValue(event->message.isNoteOn() ? event->message.getVelocity() : 0);
            
            // Send to output pin
            for (auto* connection : midiOutPin->getConnections()) {
                connection->sendEvent(e);
            }
            
            currentEventIndex++;
        }
    }
    // Handle buffer playback (recorded sequences)
    else if (buffer.getNumEvents() > 0) {
        // Use range-based for loop instead of deprecated iterator
        for (const auto metadata : buffer) {
            double eventTime = metadata.getMessage().getTimeStamp() * 0.001;

            if (eventTime <= currentTime && metadata.samplePosition >= playbackIndex) {
                Event* e = new Event("midi", Event::Type::GATE);
                e->setMessage(metadata.getMessage());
                e->setNote(metadata.getMessage().getNoteNumber());
                e->setValue(metadata.getMessage().isNoteOn() ? metadata.getMessage().getVelocity() : 0);
                
                // Send to output pin
                for (auto* connection : midiOutPin->getConnections()) {
                    connection->sendEvent(e);
                }
                
                playbackIndex = metadata.samplePosition + 1;
            } else if (eventTime > currentTime) {
                break; // Stop if events are in the future
            }
        }
    }

    // Check if playback is complete
    bool playbackComplete = false;
    if (currentSequence) {
        playbackComplete = (currentEventIndex >= currentSequence->getNumEvents());
    } else {
        playbackComplete = (playbackIndex >= buffer.getNumEvents());
    }

    if (playbackComplete) {
        stopPlaying();
    }

    repaint();
}

void MidiRecorderModule::startRecording()
{
    juce::ScopedLock sl(lock);
    
    buffer.clear();
    recordingStartTime = juce::Time::getMillisecondCounterHiRes() * 0.001;
    hasUnsavedChanges = true;

    if (recordingStateValue) {
        recordingStateValue->setValue(true);
    }
    
    repaint();
}

void MidiRecorderModule::stopRecording()
{
    juce::ScopedLock sl(lock);
    
    if (recordingStateValue) {
        recordingStateValue->setValue(false);
    }
    
    repaint();
}

void MidiRecorderModule::startPlaying()
{
    juce::ScopedLock sl(lock);
    
    if (!deviceManager) return;

    sequenceStartTime = juce::Time::getMillisecondCounterHiRes() * 0.001;
    currentEventIndex = 0;
    playbackIndex = 0;

    if (playingStateValue) {
        playingStateValue->setValue(true);
    }
    
    startTimer(1); // 1ms timer for precise playback
    repaint();
}

void MidiRecorderModule::stopPlaying()
{
    juce::ScopedLock sl(lock);
    
    stopTimer();
    
    if (playingStateValue) {
        playingStateValue->setValue(false);
    }
    
    // Send all notes off
    for (int channel = 1; channel <= 16; ++channel) {
        juce::MidiMessage allNotesOff = juce::MidiMessage::allNotesOff(channel);
        Event* e = new Event("allnotesoff", Event::Type::GATE);
        e->setMessage(allNotesOff);
        e->setValue(0);
        
        for (auto* connection : midiOutPin->getConnections()) {
            connection->sendEvent(e);
        }
    }
    
    repaint();
}

void MidiRecorderModule::triggerSequence()
{
    if (isRecording()) return; // Don't trigger while recording
    
    if (isPlaying()) {
        stopPlaying();
    } else {
        startPlaying();
    }
}

bool MidiRecorderModule::isRecording() const
{
    return recordingStateValue && recordingStateValue->getValue().toString().getIntValue() > 0;
}

bool MidiRecorderModule::isPlaying() const
{
    return playingStateValue && playingStateValue->getValue().toString().getIntValue() > 0;
}

void MidiRecorderModule::saveMidiFile()
{
    if (lastLoadedFile.existsAsFile()) {
        saveMidiFileAs(lastLoadedFile);
    } else {
        juce::FileChooser chooser("Save MIDI File", juce::File(), "*.mid");
        if (chooser.browseForFileToSave(true)) {
            saveMidiFileAs(chooser.getResult());
        }
    }
}

void MidiRecorderModule::saveMidiFileAs(const juce::File& file)
{
    juce::ScopedLock sl(lock);
    
    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(960); // Standard MIDI resolution
    
    juce::MidiMessageSequence sequence;
    
    // Convert buffer to MIDI sequence
    if (buffer.getNumEvents() > 0) {
        // Use range-based for loop instead of deprecated iterator
        for (const auto metadata : buffer) {
            // Convert sample position to time in seconds, then to MIDI ticks
            double timeInSeconds = metadata.getMessage().getTimeStamp() * 0.001;
            double timeInBeats = (timeInSeconds * tempo) / 60.0;
            double timeInTicks = timeInBeats * 960.0;
            
            juce::MidiMessage message = metadata.getMessage();
            message.setTimeStamp(timeInTicks);
            sequence.addEvent(message);
        }
    }
    // Or use current loaded sequence
    else if (currentSequence) {
        sequence = *currentSequence;
    }
    
    sequence.updateMatchedPairs();
    midiFile.addTrack(sequence);
    
    juce::FileOutputStream outputStream(file);
    if (outputStream.openedOk()) {
        midiFile.writeTo(outputStream);
        lastLoadedFile = file;
        hasUnsavedChanges = false;
        repaint();
    }
}

void MidiRecorderModule::loadMidiFile()
{
    juce::FileChooser chooser("Load MIDI File", juce::File(), "*.mid");
    if (chooser.browseForFileToOpen()) {
        loadMidiFileFrom(chooser.getResult());
    }
}

void MidiRecorderModule::loadMidiFileFrom(const juce::File& file)
{
    juce::ScopedLock sl(lock);
    
    stopPlaying();
    stopRecording();
    
    juce::FileInputStream inputStream(file);
    if (inputStream.openedOk()) {
        currentMidiFile.readFrom(inputStream);
        
        if (currentMidiFile.getNumTracks() > 0) {
            // Use the first track
            if (currentSequence) {
                delete currentSequence;
            }
            
            currentSequence = new juce::MidiMessageSequence(*currentMidiFile.getTrack(0));
            
            // Clear the buffer since we're now using a MIDI file
            buffer.clear();
            
            lastLoadedFile = file;
            hasUnsavedChanges = false;
            
            // Extract tempo if available
            for (int i = 0; i < currentSequence->getNumEvents(); ++i) {
                const auto* event = currentSequence->getEventPointer(i);
                if (event->message.isTempoMetaEvent()) {
                    double newTempo = 60.0 / event->message.getTempoSecondsPerQuarterNote();
                    setTempo(static_cast<float>(newTempo));
                    break;
                }
            }
        }
    }
    
    repaint();
}

void MidiRecorderModule::clearSequence()
{
    juce::ScopedLock sl(lock);
    
    stopPlaying();
    stopRecording();
    
    buffer.clear();
    
    if (currentSequence) {
        delete currentSequence;
        currentSequence = nullptr;
    }
    
    currentMidiFile.clear();
    lastLoadedFile = juce::File();
    hasUnsavedChanges = false;
    
    repaint();
}

void MidiRecorderModule::setDeviceManager(juce::AudioDeviceManager* manager)
{
    deviceManager = manager;
}

void MidiRecorderModule::setTempo(float newTempo)
{
    tempo = newTempo;
    if (tempoValue) {
        tempoValue->setValue(newTempo);
    }
    repaint();
}

void MidiRecorderModule::createProperties()
{
    tempoValue = new juce::Value();
    tempoValue->setValue(120.0f);
    tempoListener = new TempoListener(*tempoValue, this);

    recordingStateValue = new juce::Value();
    recordingStateListener = new RecordingStateListener(*recordingStateValue, this);

    playingStateValue = new juce::Value();
    playingStateListener = new PlayingStateListener(*playingStateValue, this);
}

void MidiRecorderModule::eventReceived(Event* e)
{
    if (!e) return;
    
    // Handle trigger events on trigger pin
    if (e->getType() == Event::Type::GATE && e->getValue() > 0) {
        // Simple approach: if we have connections to the trigger pin, assume this is a trigger
        if (triggerPin && !triggerPin->getConnections().empty()) {
            triggerSequence();
            return;
        }
    }
    
    // Handle MIDI input for recording
    if (isRecording() && (e->getMessage().isController() || e->getMessage().isNoteOnOrOff())) {
        double currentTime = juce::Time::getMillisecondCounterHiRes() * 0.001;
        double relativeTime = currentTime - recordingStartTime;
        
        juce::MidiMessage msg = e->getMessage();
        msg.setTimeStamp(relativeTime * 1000.0); // Store in milliseconds
        
        buffer.addEvent(msg, static_cast<int>(relativeTime * 1000.0));
        hasUnsavedChanges = true;
        repaint();
    }
}

void MidiRecorderModule::handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message)
{
    // Always pass through MIDI messages
    Event* e = new Event("midi", Event::Type::GATE);
    e->setMessage(message);
    e->setNote(message.getNoteNumber());
    e->setValue(message.isNoteOn() ? message.getVelocity() : 0);
    
    for (auto* connection : midiOutPin->getConnections()) {
        connection->sendEvent(e);
    }

    // Record if recording is active
    if (isRecording()) {
        double currentTime = juce::Time::getMillisecondCounterHiRes() * 0.001;
        double relativeTime = currentTime - recordingStartTime;
        
        juce::MidiMessage msg = message;
        msg.setTimeStamp(relativeTime * 1000.0); // Store in milliseconds
        
        buffer.addEvent(msg, static_cast<int>(relativeTime * 1000.0));
        hasUnsavedChanges = true;
    }
    
    repaint();
}

juce::Array<juce::PropertyComponent*>& MidiRecorderModule::getProperties()
{
    properties = juce::Array<juce::PropertyComponent*>();

    tempoProperty = new juce::SliderPropertyComponent(*tempoValue, "Tempo", 60, 300, 0.1, 1.0, true);
    recordingStateProperty = new juce::BooleanPropertyComponent(*recordingStateValue, "Recording", "Recording");
    playingStateProperty = new juce::BooleanPropertyComponent(*playingStateValue, "Playing", "Playing");

    properties.add(tempoProperty);
    properties.add(recordingStateProperty);
    properties.add(playingStateProperty);
    return properties;
}

void MidiRecorderModule::updateFileDisplay()
{
    repaint();
}

juce::String MidiRecorderModule::getCurrentFileName() const
{
    if (lastLoadedFile.existsAsFile()) {
        return lastLoadedFile.getFileNameWithoutExtension();
    }
    return "Untitled";
}

// Add mouse handling for right-click menu
void MidiRecorderModule::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isRightButtonDown()) {
        juce::PopupMenu menu;
        
        menu.addItem(1, "Load MIDI File...");
        menu.addItem(2, "Save MIDI File");
        menu.addItem(3, "Save MIDI File As...");
        menu.addSeparator();
        menu.addItem(4, "Clear Sequence");
        menu.addSeparator();
        menu.addItem(5, "Record", true, isRecording());
        menu.addItem(6, "Play", true, isPlaying());
        menu.addItem(7, "Stop");
        
        int result = menu.show();
        
        switch (result) {
            case 1: loadMidiFile(); break;
            case 2: saveMidiFile(); break;
            case 3: {
                juce::FileChooser chooser("Save MIDI File As", juce::File(), "*.mid");
                if (chooser.browseForFileToSave(true)) {
                    saveMidiFileAs(chooser.getResult());
                }
                break;
            }
            case 4: clearSequence(); break;
            case 5: 
                if (isRecording()) stopRecording();
                else startRecording();
                break;
            case 6: 
                if (isPlaying()) stopPlaying();
                else startPlaying();
                break;
            case 7: 
                stopPlaying();
                stopRecording();
                break;
        }
    } else {
        Module::mouseDown(e);
    }
}
