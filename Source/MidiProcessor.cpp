#include "MidiProcessor.h"

MidiProcessor::MidiProcessor(SynthEditor* root, AudioDeviceManager* deviceManager)
{
	this->editor = root;
	this->deviceManager = deviceManager;
}

MidiProcessor::~MidiProcessor()
{
}

void MidiProcessor::handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) {
	ScopedLock cl(lock);
	//Logger::getCurrentLogger()->writeToLog(message.getDescription() + " at time " + String(Time::getMillisecondCounter()));

	for (int i = 0; i < this->editor->getModule()->getModules()->size(); i++) {

		MidiRecorderModule* recorder = dynamic_cast<MidiRecorderModule*>(this->editor->getModule()->getModules()->at(i));

		if (recorder != nullptr) {
			recorder->handleIncomingMidiMessage(source, message);
		}

		MidiGate* gate = dynamic_cast<MidiGate*>(this->editor->getModule()->getModules()->at(i));

		if (gate != nullptr) {
			sendMidiMessage(gate, message);
		}

	}
	
	if (message.isNoteOn() && message.getNoteNumber() > 7) {
		for (int i = 0; i < this->editor->getModule()->getModules()->size(); i++) {
			sendNoteMessage(this->editor->getModule()->getModules()->at(i), message.getChannel(), message.getNoteNumber());
			sendGateMessage(this->editor->getModule()->getModules()->at(i), message.getChannel(), message.getVelocity(), message.getNoteNumber(), true);
		}
	}
	else if (message.isNoteOff() && message.getNoteNumber() > 7) {
		for (int i = 0; i < this->editor->getModule()->getModules()->size(); i++) {
			sendGateMessage(this->editor->getModule()->getModules()->at(i), message.getChannel(), message.getVelocity(), message.getNoteNumber(), false);
		}
	}
	else if (message.isController()) {
		for (int i = 0; i < this->editor->getModule()->getModules()->size(); i++) {
			sendControllerMessage(this->editor->getModule()->getModules()->at(i), message.getChannel(), message.getControllerNumber(), message.getControllerValue());
		}
	}
	else if (message.isPitchWheel()) {
		for (int i = 0; i < this->editor->getModule()->getModules()->size(); i++) {
			sendPitchBendMessage(this->editor->getModule()->getModules()->at(i), message.getChannel(), message.getPitchWheelValue());
		}
	}
	else if (message.isMidiClock()) {
		for (int i = 0; i < this->editor->getModule()->getModules()->size(); i++) {
			sendClockMessage(this->editor->getModule()->getModules()->at(i), message.getChannel());
		}
	}
	else if (message.isMidiStart()) {
		for (int i = 0; i < this->editor->getModule()->getModules()->size(); i++) {
			sendMidiStartMessage(this->editor->getModule()->getModules()->at(i), message.getChannel());
		}
	}
	else if (message.isMidiStop()) {
		for (int i = 0; i < this->editor->getModule()->getModules()->size(); i++) {
			sendMidiStopMessage(this->editor->getModule()->getModules()->at(i), message.getChannel());
		}
	}
	else if (message.isMidiMachineControlMessage()) {

		MidiMessage::MidiMachineControlCommand command = message.getMidiMachineControlCommand();

		switch (command)
		{
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

	// deviceManager.getDefaultMidiOutput()->sendMessageNow(message);
	
}

void MidiProcessor::sendGateMessage(Module* module, int channel, int velocity, int note, bool on) {

	MidiGate* gate;

	if ((gate = dynamic_cast<MidiGate*>(module)) != nullptr) {
		if (gate->getChannel() == channel) {
			if (on) {
				if (velocity > 0)
					gate->gateOn(velocity, note);
			}
			else {
				gate->gateOff(note);
			}
		}
	}

	for (int i = 0; i < module->getModules()->size(); i++) {

		if ((gate = dynamic_cast<MidiGate*>(module->getModules()->at(i))) != nullptr) {
			if (gate->getChannel() == channel) {
				if (on) {
					gate->gateOn(velocity, note);
				}
				else {
					gate->gateOff(note);
				}

				sendGateMessage(module->getModules()->at(i), channel, velocity, note, on);
			}
		}
	}
}

void MidiProcessor::sendNoteMessage(Module* module, int channel, int note) {

	MidiNote* midiNote;

	if ((midiNote = dynamic_cast<MidiNote*>(module)) != nullptr) {
		if (note > 0)
			midiNote->note(note);
	}

	for (int i = 0; i < module->getModules()->size(); i++) {

		if ((midiNote = dynamic_cast<MidiNote*>(module->getModules()->at(i))) != nullptr) {
			sendNoteMessage(module->getModules()->at(i), channel, note);
		}
	}
}

void MidiProcessor::sendClockMessage(Module* module, int channel) {

	MidiClock* midiClock;

	if ((midiClock = dynamic_cast<MidiClock*>(module)) != nullptr) {
		midiClock->clock(MidiMessage::midiClock());
	}

	for (int i = 0; i < module->getModules()->size(); i++) {

		if ((midiClock = dynamic_cast<MidiClock*>(module->getModules()->at(i))) != nullptr) {
			sendClockMessage(module->getModules()->at(i), channel);
		}
	}
}

void MidiProcessor::sendMidiStartMessage(Module* module, int channel) {

	MidiClock* midiClock;

	if ((midiClock = dynamic_cast<MidiClock*>(module)) != nullptr) {
		midiClock->midiStart();
	}

	for (int i = 0; i < module->getModules()->size(); i++) {

		if ((midiClock = dynamic_cast<MidiClock*>(module->getModules()->at(i))) != nullptr) {
			sendMidiStartMessage(module->getModules()->at(i), channel);
		}
	}
}

void MidiProcessor::sendMidiStopMessage(Module* module, int channel) {

	MidiClock* midiClock;

	if ((midiClock = dynamic_cast<MidiClock*>(module)) != nullptr) {
		midiClock->midiStop();
	}

	for (int i = 0; i < module->getModules()->size(); i++) {

		if ((midiClock = dynamic_cast<MidiClock*>(module->getModules()->at(i))) != nullptr) {
			sendMidiStopMessage(module->getModules()->at(i), channel);
		}
	}
}

void MidiProcessor::sendPitchBendMessage(Module* module, int channel, int pitch) {

	PitchBendModule* pbm;

	if ((pbm = dynamic_cast<PitchBendModule*>(module)) != nullptr) {
		pbm->setPitch(pitch);
	}

	for (int i = 0; i < module->getModules()->size(); i++) {
		if ((pbm = dynamic_cast<PitchBendModule*>(module->getModules()->at(i))) != nullptr) {
			sendPitchBendMessage(module->getModules()->at(i), channel, pitch);
		}
	}
}

void MidiProcessor::sendControllerMessage(Module* module, int channel, int controller, float value) {

	Knob* k;

	if ((k = dynamic_cast<Knob*>(module)) != nullptr) {
		if (k->isLearning()) {
			k->setIsMidicontroller(true);
			k->setController(controller);
			k->setLearning(false);
		}
		if (k->getController() == controller) {
			k->setValue(value);
		}

	}
	for (int i = 0; i < module->getModules()->size(); i++) {
		if ((k = dynamic_cast<Knob*>(module->getModules()->at(i))) != nullptr) {
			sendControllerMessage(module->getModules()->at(i), channel, controller, value);
		}
	}
}

void MidiProcessor::refreshMidiInputs() {
	for (int i = 0; i < MidiInput::getDevices().size(); i++) {
		if (deviceManager->isMidiInputEnabled(MidiInput::getDevices().getReference(i))) {
			deviceManager->removeMidiInputCallback(MidiInput::getDevices().getReference(i), this);
			deviceManager->addMidiInputCallback(MidiInput::getDevices().getReference(i), this);
		}
	}
}

void MidiProcessor::enableAllMidiInputs() {
	for (int i = 0; i < MidiInput::getDevices().size(); i++) {
		if (deviceManager->isMidiInputEnabled(MidiInput::getDevices().getReference(i))) {
			deviceManager->addMidiInputCallback(MidiInput::getDevices().getReference(i), this);
		}
	}
}

void MidiProcessor::disableAllMidiInputs() {
	for (int i = 0; i < MidiInput::getDevices().size(); i++) {
		if (deviceManager->isMidiInputEnabled(MidiInput::getDevices().getReference(i))) {
			deviceManager->removeMidiInputCallback(MidiInput::getDevices().getReference(i), this);
		}
	}
}

void MidiProcessor::sendMidiMessage(Module* module, const juce::MidiMessage& message)
{
	MidiGate* gate;

	if ((gate = dynamic_cast<MidiGate*>(module)) != nullptr) {
		Event* e = new Event();
		e->setMessage(message);
		gate->eventReceived(e);
	}
}
