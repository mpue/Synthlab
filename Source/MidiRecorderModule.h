#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "MidiRecorder/MidiRecorder.h"
#include "Module.h"

//==============================================================================
// Enhanced MidiRecorderModule with MIDI file support and trigger input
class MidiRecorderModule : public Module, public juce::Timer
{
public:
	MidiRecorderModule();
	~MidiRecorderModule() override;

	void configurePins() override;
	void paint(juce::Graphics& g) override;

	void timerCallback() override;

	void startRecording();
	void stopRecording();
	void startPlaying();
	void stopPlaying();

	bool isRecording() const;
	bool isPlaying() const;

	void setDeviceManager(juce::AudioDeviceManager* manager);
	void setTempo(float newTempo);

	// MIDI file functionality
	void saveMidiFile();
	void saveMidiFileAs(const juce::File& file);
	void loadMidiFile();
	void loadMidiFileFrom(const juce::File& file);
	void clearSequence();

	// Trigger functionality
	void triggerSequence();

	juce::Array<juce::PropertyComponent*>& getProperties() override;
	void createProperties() override;
	void eventReceived(Event* e) override;
	void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message);

	juce::String getCategory() override { return "MIDI"; }

	// Mouse handling for context menu
	void mouseDown(const juce::MouseEvent& e) override;

private:

	struct TempoListener : juce::Value::Listener
	{
		TempoListener(juce::Value& v, MidiRecorderModule* g) : g(g), value(v) { value.addListener(this); }
		~TempoListener() {}

		void valueChanged(juce::Value& value) override {
			g->setTempo(value.toString().getFloatValue());
		}
		MidiRecorderModule* g;
		juce::Value value;
	};

	struct RecordingStateListener : juce::Value::Listener
	{
		RecordingStateListener(juce::Value& v, MidiRecorderModule* g) : g(g), value(v) { value.addListener(this); }
		~RecordingStateListener() {}

		void valueChanged(juce::Value& value) override {
			if (value.toString().getIntValue() > 0) {
				g->stopPlaying();
				g->startRecording();
			}
			else {
				g->stopRecording();
			}
		}
		MidiRecorderModule* g;
		juce::Value value;
	};

	struct PlayingStateListener : juce::Value::Listener
	{
		PlayingStateListener(juce::Value& v, MidiRecorderModule* g) : g(g), value(v) { value.addListener(this); }
		~PlayingStateListener() {}

		void valueChanged(juce::Value& value) override {
			if (value.toString().getIntValue() > 0) {
				g->stopRecording();
				g->startPlaying();
			}
			else {
				g->stopPlaying();
			}			
		}
		MidiRecorderModule* g;
		juce::Value value;
	};

	// File management
	void updateFileDisplay();
	juce::String getCurrentFileName() const;

	juce::AudioDeviceManager* deviceManager = nullptr;

	Pin* midiInPin = nullptr;
	Pin* midiOutPin = nullptr;
	Pin* triggerPin = nullptr;  // New trigger input

	juce::MidiBuffer buffer;
	juce::MidiFile currentMidiFile;  // Current MIDI file
	juce::File lastLoadedFile;       // Track loaded file
	bool hasUnsavedChanges = false;  // Track if sequence has changes

	juce::Value* tempoValue;
	juce::Value* recordingStateValue;
	juce::Value* playingStateValue;

	TempoListener* tempoListener = nullptr;
	RecordingStateListener* recordingStateListener = nullptr;
	PlayingStateListener* playingStateListener = nullptr;

	juce::PropertyComponent* tempoProperty = nullptr;
	juce::PropertyComponent* recordingStateProperty = nullptr;
	juce::PropertyComponent* playingStateProperty = nullptr;

	juce::Array<juce::PropertyComponent*> properties;

	float tempo = 120.0f;
	double startTime = 0.0;
	double recordingStartTime = 0.0;

	int playbackIndex = 0;
	// Removed deprecated MidiBuffer::Iterator

	// For MIDI file playback
	juce::MidiMessageSequence* currentSequence = nullptr;
	int currentEventIndex = 0;
	double sequenceStartTime = 0.0;

	juce::CriticalSection lock;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiRecorderModule)
};
