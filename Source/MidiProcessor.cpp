#include "MidiProcessor.h"

MidiProcessor::MidiProcessor(SynthEditor* root, AudioDeviceManager* deviceManager)
{
	this->editor = root;
	this->deviceManager = deviceManager;
}

MidiProcessor::~MidiProcessor() {}

void MidiProcessor::handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) {
	ScopedLock cl(lock);

	const int midiInChannel = message.getChannel(); // Eingehender MIDI-Kanal
	const int targetChannel = activeChannel;        // Ziel-Track-Kanal (per TrackNavigator gesetzt)

	auto* modules = editor->getModule()->getModules();

	// 🎹 Alle Recorder-Module bekommen IMMER alles
	for (auto* mod : *modules) {
		if (auto* recorder = dynamic_cast<MidiRecorderModule*>(mod)) {
			recorder->handleIncomingMidiMessage(source, message);
		}
	}

	if (message.isNoteOn() && message.getNoteNumber() > 7) {
		for (auto* mod : *modules) {
			Logger::writeToLog("Note ON " + String(message.getNoteNumber()) + " to channel " + String(targetChannel));
			sendNoteMessage(mod, targetChannel, message.getNoteNumber());
			sendGateMessage(mod, targetChannel, message.getVelocity(), message.getNoteNumber(), true);
		}
	}
	else if (message.isNoteOff() && message.getNoteNumber() > 7) {
		for (auto* mod : *modules) {
			sendGateMessage(mod, targetChannel, message.getVelocity(), message.getNoteNumber(), false);
		}
	}
	else if (message.isController()) {
		for (auto* mod : *modules) {
			// 🎛 Controller wird gezielt an Ziel-Track (activeChannel) geschickt
			sendControllerMessage(mod, targetChannel, message.getControllerNumber(), message.getControllerValue(), midiInChannel);
		}
	}
	else if (message.isPitchWheel()) {
		for (auto* mod : *modules) {
			sendPitchBendMessage(mod, targetChannel, message.getPitchWheelValue());
		}
	}
	else if (message.isMidiStart()) {
		for (auto* mod : *modules) {
			sendMidiStartMessage(mod, targetChannel);
		}
	}
	else if (message.isMidiStop()) {
		for (auto* mod : *modules) {
			sendMidiStopMessage(mod, targetChannel);
		}
	}
	else if (message.isMidiMachineControlMessage()) {
		auto command = message.getMidiMachineControlCommand();
		switch (command) {
		case MidiMessage::MidiMachineControlCommand::mmc_play:
			Logger::writeToLog("mmc_play");
			break;
		case MidiMessage::MidiMachineControlCommand::mmc_stop:
			Logger::writeToLog("mmc_stop");
			break;
		default:
			break;
		}
	}
	else {
		Logger::writeToLog(message.getDescription());
	}
}

void MidiProcessor::sendGateMessage(Module* module, int channel, int velocity, int note, bool on) {
	if (auto* gate = dynamic_cast<MidiGate*>(module)) {
		if (gate->getChannel() == channel) {
			if (on && velocity > 0)
				gate->gateOn(velocity, note);
			else
				gate->gateOff(note);
		}
	}
	for (auto* sub : *module->getModules()) {
		sendGateMessage(sub, channel, velocity, note, on);
	}
}

void MidiProcessor::sendNoteMessage(Module* module, int channel, int note) {
	if (auto* midiNote = dynamic_cast<MidiNote*>(module)) {
		if (note > 0)
			midiNote->note(note);
	}
	for (auto* sub : *module->getModules()) {
		sendNoteMessage(sub, channel, note);
	}
}

void MidiProcessor::sendPitchBendMessage(Module* module, int channel, int pitch) {
	if (auto* pbm = dynamic_cast<PitchBendModule*>(module)) {
		// if (pbm->getChannel() == channel)
			pbm->setPitch(pitch);
	}
	for (auto* sub : *module->getModules()) {
		sendPitchBendMessage(sub, channel, pitch);
	}
}

void MidiProcessor::sendControllerMessage(Module* module, int targetChannel, int controller, float value, int midiInChannel) {
	if (auto* knob = dynamic_cast<Knob*>(module)) {
		if (knob->isLearning()) {
			// Nur lernen, wenn Kanal passt oder explizit erlaubt
			if (true /* oder z.B. knob->learnsFromChannel(midiInChannel) */) {
				knob->setIsMidicontroller(true);
				knob->setController(controller);
				knob->setLearning(false);
			}
		}
		if (knob->getController() == controller) {
			knob->setValue(value);
		}
	}
	for (auto* sub : *module->getModules()) {
		sendControllerMessage(sub, targetChannel, controller, value, midiInChannel);
	}
}

void MidiProcessor::sendMidiStartMessage(Module* module, int channel) {
	if (auto* midiClock = dynamic_cast<MidiClock*>(module)) {
		midiClock->midiStart();
	}
	for (auto* sub : *module->getModules()) {
		sendMidiStartMessage(sub, channel);
	}
}

void MidiProcessor::sendMidiStopMessage(Module* module, int channel) {
	if (auto* midiClock = dynamic_cast<MidiClock*>(module)) {
		midiClock->midiStop();
	}
	for (auto* sub : *module->getModules()) {
		sendMidiStopMessage(sub, channel);
	}
}

void MidiProcessor::refreshMidiInputs() {
	auto inputs = MidiInput::getDevices();
	for (const auto& input : inputs) {
		if (deviceManager->isMidiInputEnabled(input)) {
			deviceManager->removeMidiInputCallback(input, this);
			deviceManager->addMidiInputCallback(input, this);
		}
	}
}

void MidiProcessor::enableAllMidiInputs() {
	auto inputs = MidiInput::getDevices();
	for (const auto& input : inputs) {
		if (deviceManager->isMidiInputEnabled(input) && input.contains("SL")) {
			deviceManager->addMidiInputCallback(input, this);
		}
	}
}

void MidiProcessor::disableAllMidiInputs() {
	auto inputs = MidiInput::getDevices();
	for (const auto& input : inputs) {
		if (deviceManager->isMidiInputEnabled(input)) {
			deviceManager->removeMidiInputCallback(input, this);
		}
	}
}
