/*
  ==============================================================================

	MidiTools.h
	Created: 24 Jan 2025 9:59:33pm
	Author:  mpue

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "MidiRecorderModule.h"
#include "MidiNote.h"
#include "MidiGate.h"
#include "MidiClock.h"
#include "PitchBendModule.h"
#include "Knob.h"
#include "Module.h"
#include "SynthEditor.h"

class MidiProcessor : public juce::MidiInputCallback
{
public:

	MidiProcessor(SynthEditor* editor, AudioDeviceManager* deviceManager);
	~MidiProcessor();

	virtual void handleIncomingMidiMessage(MidiInput* source, const MidiMessage& message)override;

	void sendGateMessage(Module* module, int channel, int velocity, int note, bool on);
	void sendNoteMessage(Module* module, int channel, int note);
	void sendClockMessage(Module* module, int channel);
	void sendMidiStartMessage(Module* module, int channel);
	void sendMidiStopMessage(Module* module, int channel);
	void sendControllerMessage(Module* module, int channel, int controller, float value, int midiInChannel);
	void sendPitchBendMessage(Module* module, int channel, int pitch);

	void refreshMidiInputs();
	void enableAllMidiInputs();
	void disableAllMidiInputs();

	void setActiveChannel(int channel) {
		activeChannel = channel;
	}

	void sendMidiMessage(Module* module, const juce::MidiMessage& message);

private:
	CriticalSection lock;
	SynthEditor* editor;
	AudioDeviceManager* deviceManager;
	int activeChannel = 1;
};
