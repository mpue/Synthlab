/*  MidiProcessor.cpp
 *  Queue-based implementation
 */

#include "MidiProcessor.h"
#include "SynthEditor.h"          // your own header
#include "Module.h"               // base module
#include "MidiRecorderModule.h"   // recorder
#include "MidiGate.h"
#include "MidiNote.h"
#include "PitchBendModule.h"
#include "MidiClock.h"
#include "Knob.h"                // for MIDI controller knobs
#include "PluginModule.h"         // for plugin modules


using namespace juce;

MidiProcessor::MidiProcessor(SynthEditor* root,
	AudioDeviceManager* dm)
	: editor(root),
	deviceManager(dm)
{
	// 5 ms period ⇒ 200 Hz; adjust if needed
	startTimer(2);
	setMidiClockBPM(120.0);
	startMidiClock();

}

/* 1) High-priority MIDI callback – enqueue only, never heavy work! */
void MidiProcessor::handleIncomingMidiMessage(MidiInput* /*source*/,
	const MidiMessage& message)
{
	const double tsSec = Time::getMillisecondCounterHiRes() * 0.001;

	std::lock_guard<std::mutex> g(queueMutex);
	midiQueue.emplace_back(message, tsSec);
}

/* 2) Timer thread – dequeues and calls the original routing logic. */
void MidiProcessor::timerCallback()
{
	std::vector<QueuedMidiMessage> local;
	{
		std::lock_guard<std::mutex> g(queueMutex);
		std::swap(local, midiQueue);
	}

	for (const auto& qm : local)
		processMidiMessage(qm.msg);

	// MIDI-Clock-Tick senden, wenn aktiv
	if (clockRunning)
	{
		const auto now = Time::getMillisecondCounter();
		if (now - lastClockTimeMs >= clockIntervalMs)
		{
			for (auto* mod : *editor->getModule()->getModules())
			{
				sendMidiClockMessage(mod, activeChannel.load());

			}
			lastClockTimeMs = now;
		}
	}

}

void MidiProcessor::processMidiMessage(const juce::MidiMessage& msg)
{
	const int targetChannel = activeChannel.load();
	auto* modules = editor->getModule()->getModules();

	// Recorder first (unverändert)
	for (auto* mod : *modules)
		if (auto* rec = dynamic_cast<MidiRecorderModule*>(mod))
			rec->handleIncomingMidiMessage(nullptr, msg);

	//------------------------------------------------------------------
	// 1.  NOTE-ON  (Velocity > 0)
	//------------------------------------------------------------------
	if (msg.isNoteOn() && msg.getVelocity() > 0.0f && msg.getNoteNumber() > 7)
	{
		for (auto* mod : *modules)
		{
			sendNoteMessage(mod, targetChannel, msg.getNoteNumber());
			sendGateMessage(mod, targetChannel, msg.getVelocity(),
				msg.getNoteNumber(), /*on=*/true);
		}
		return; // fertig
	}

	//------------------------------------------------------------------
	// 2.  NOTE-OFF  (klassisch)  **oder**  NOTE-ON mit Vel == 0
	//------------------------------------------------------------------
	if ((msg.isNoteOff() || (msg.isNoteOn() && msg.getVelocity() == 0.0f))
		&& msg.getNoteNumber() > 7)
	{
		for (auto* mod : *modules)
			sendGateMessage(mod, targetChannel, msg.getVelocity(),
				msg.getNoteNumber(), /*on=*/false);
		return; // fertig
	}

	//------------------------------------------------------------------
	// 3.  CONTROLLER / PITCH / CLOCK / START / STOP  (unverändert)
	//------------------------------------------------------------------
	if (msg.isController())
		for (auto* mod : *modules)
			sendControllerMessage(mod, targetChannel,
				msg.getControllerNumber(),
				msg.getControllerValue(),
				msg.getChannel());

	else if (msg.isPitchWheel())
		for (auto* mod : *modules)
			sendPitchBendMessage(mod, targetChannel,
				msg.getPitchWheelValue());

	else if (msg.isMidiStart())
		for (auto* mod : *modules)
			sendMidiStartMessage(mod, targetChannel);

	else if (msg.isMidiStop())
		for (auto* mod : *modules)
			sendMidiStopMessage(mod, targetChannel);
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
	else if(auto* plugin = dynamic_cast<PluginModule*>(module)) {
		MidiMessage msg = MidiMessage::controllerEvent(
			midiInChannel, controller, static_cast<int>(value));
		Event* event = new Event("controller", Event::Type::CONTROLLER);
		event->setMessage(msg);
		plugin->eventReceived(event);
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

void MidiProcessor::sendMidiClockMessage(Module* module, int channel)
{
	if (auto* midiClock = dynamic_cast<MidiClock*>(module)) {
		midiClock->clock(MidiMessage::midiClock());
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

void MidiProcessor::startMidiClock()
{
	clockRunning = true;
	lastClockTimeMs = Time::getMillisecondCounter();
}

void MidiProcessor::stopMidiClock()
{
	clockRunning = false;
}

void MidiProcessor::setMidiClockBPM(double bpm)
{
	clockBPM = bpm;
	clockIntervalMs = static_cast<int>(60000.0 / (24.0 * clockBPM));
}

bool MidiProcessor::isMidiClockRunning() const
{
	return clockRunning;
}


void MidiProcessor::processBlock(int numSamples, TrackNavigator* navigator)
{

	const int currentSamplePos = navigator->getSamplePosition();
	const auto& tracks = navigator->getTracks();

	for (auto* track : tracks)
	{
		if (track == nullptr)
			continue;

		for (auto* region : track->getRegions())
		{
			auto* midiRegion = dynamic_cast<MidiRegion*>(region);
			
			if (midiRegion == nullptr)
				continue;

			vector <MidiMessage*> messages = midiRegion->getMessages(currentSamplePos, numSamples);

			for (auto* message : messages)
			{ 
				if (message != nullptr)
					processMidiMessage(*message);
			}

		}
	}
}

