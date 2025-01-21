#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "MidiRecorder/MidiRecorder.h"
#include "Module.h"

//==============================================================================
// MidiRecorderModule: Erweiterung des MidiRecorder für ein modulares System
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

	juce::Array<juce::PropertyComponent*>& getProperties() override;
	void createProperties() override;
	void eventReceived(Event* e) override;
	void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message);

private:

	struct TempoListener : juce::Value::Listener
	{
		TempoListener(juce::Value& v, MidiRecorderModule* g) : g(g), value(v) { value.addListener(this); }
		~TempoListener() {}  // no need to remove the listener

		void valueChanged(juce::Value& value) override {
			g->setTempo(value.toString().getFloatValue());
		}
		MidiRecorderModule* g;
		juce::Value value;
	};

	struct RecordingStateListener : juce::Value::Listener
	{
		RecordingStateListener(juce::Value& v, MidiRecorderModule* g) : g(g), value(v) { value.addListener(this); }
		~RecordingStateListener() {}  // no need to remove the listener

		void valueChanged(juce::Value& value) override {
			if (value.toString().getIntValue() > 0) {
				g->stopPlaying();
				g->startRecording();
			}
			else {
				g->stopRecording();
			}
			// g->setRecordingState(value.toString().getIntValue() > 0 ? true : false);
		}
		MidiRecorderModule* g;
		juce::Value value;
	};

	struct PlayingStateListener : juce::Value::Listener
	{
		PlayingStateListener(juce::Value& v, MidiRecorderModule* g) : g(g), value(v) { value.addListener(this); }
		~PlayingStateListener() {}  // no need to remove the listener

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

	// MidiRecorder midiRecorder;
	juce::AudioDeviceManager* deviceManager = nullptr;

	Pin* midiInPin = nullptr;
	Pin* midiOutPin = nullptr;

	juce::MidiBuffer buffer;

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

	float tempo = 120.0f; // Standardtempo in BPM
	int startTime = 0;

	int playbackIndex = 0;
	juce::MidiBuffer::Iterator* playbackIterator = nullptr;

	CriticalSection lock;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiRecorderModule)
};
