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
#include "TrackIn.h"
#include "Connection.h"           // for gate->TrackIn graph walk
#include "Tracks/Track.h"
#include "MessageBus/FastBus.h"
#include "Project/Project.h"       // Transport-Tempo / Sample-Rate fuer die Clock

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
	auto& bus = FastBus::getInstance();

	bus.subscribe<bool>("running", [this](const FastTopic<bool>& topic) {
		running = topic.getValue();
		if (!running) {
			// Clear MIDI queue when stopping
			std::lock_guard<std::mutex> g(queueMutex);
			midiQueue.clear();
			stopTimer();
		}
		if (!isTimerRunning() && running) {
			startTimer(2);
		}
	});

	// Transport-Gate: Clock nur waehrend Play/Record ticken lassen.
	bus.subscribe<bool>("transport_active", [this](const FastTopic<bool>& topic) {
		const bool active = topic.getValue();
		const bool wasActive = transportActive.exchange(active);
		if (active && !wasActive) {
			// Tick-Zaehler an die aktuelle Playhead-Position koppeln (kein
			// Backfill), damit Step 1 zur Transport-Position passt.
			clockResyncPending = true;
			// CLOCK_START zentral ueber ALLE Play/Record-Wege (nicht nur ueber
			// die Toolbar) an die Clock-Kette schicken -> Sequencer setzen
			// taktbuendig auf Step 1. sendMidiStartMessage ist idempotent, das
			// zusaetzliche Toolbar-CLOCK_START schadet also nicht.
			if (editor != nullptr && editor->getModule())
				for (auto* mod : *editor->getModule()->getModules())
					sendMidiStartMessage(mod, activeChannel.load());
		}
		else if (!active && wasActive) {
			if (editor != nullptr && editor->getModule())
				for (auto* mod : *editor->getModule()->getModules())
					sendMidiStopMessage(mod, activeChannel.load());
		}
	});

	// Aktuelle Geraete als "bekannt" markieren — beim Start respektieren wir
	// die im config.xml gespeicherte Enabled-Auswahl (enableAllMidiInputs),
	// nur spaeter neu eingesteckte Geraete sollen automatisch aktiv werden.
	for (const auto& input : MidiInput::getAvailableDevices())
		knownInputIdentifiers.add(input.identifier);

	// Auf Hotplug der MIDI-Geraeteliste lauschen. Der Callback feuert auf dem
	// Message-Thread, daher duerfen wir den deviceManager direkt anfassen.
	midiDeviceListConnection = MidiDeviceListConnection::make(
		[this] { handleMidiDeviceListChange(); });
}

void MidiProcessor::handleMidiDeviceListChange()
{
	auto inputs = MidiInput::getAvailableDevices();

	for (const auto& input : inputs)
	{
		if (knownInputIdentifiers.contains(input.identifier))
			continue;

		// Neu eingestecktes Geraet -> automatisch scharf schalten und den
		// Callback registrieren, damit sofort Noten durchkommen.
		deviceManager->setMidiInputDeviceEnabled(input.identifier, true);
		deviceManager->removeMidiInputDeviceCallback(input.identifier, this);
		deviceManager->addMidiInputDeviceCallback(input.identifier, this);
	}

	// Bekannte Liste neu aufbauen: abgezogene Geraete fliegen raus, damit ein
	// spaeteres Wiedereinstecken erneut als "neu" erkannt und aktiviert wird.
	knownInputIdentifiers.clearQuick();
	for (const auto& input : inputs)
		knownInputIdentifiers.add(input.identifier);
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
	if (!running)
		return;

	std::vector<QueuedMidiMessage> local;
	{
		std::lock_guard<std::mutex> g(queueMutex);
		std::swap(local, midiQueue);
	}

	for (const auto& qm : local)
		processMidiMessage(qm.msg);

	// Die MIDI-Clock-Ticks werden NICHT mehr hier (Message-Thread) erzeugt,
	// sondern sample-genau im Audio-Callback (generateClockTicks), aufgerufen
	// aus MainComponent::getNextAudioBlock. Der Message-Thread wird von GUI-Last
	// (Tooltips/Repaints) verstopft -> verspaetete/gebuendelte Ticks -> Drift.
	// Der Audio-Thread ist echtzeitfaehig und haelt ohnehin den Graph-Lock.
}

void MidiProcessor::generateClockTicks(juce::int64 transportSamplePos)
{
	// Laeuft auf dem AUDIO-Thread, innerhalb des graphLocks von
	// getNextAudioBlock — daher sicher gegen gleichzeitige Graph-Edits und
	// sample-genau zur Transport-Position. Die Soll-Anzahl 24-PPQN-Ticks ist
	// floor(playhead / samplesPerTick); Tempo aus Project::getTempo(), Phase an
	// Sample 0. So bleibt der Sequencer driftfrei am Transport.
	if (!clockRunning || !transportActive)
		return;

	const double bpm = Project::getInstance()->getTempo();
	const double sr  = Project::getInstance()->getSampleRate();
	if (bpm <= 0.0 || sr <= 0.0)
		return;

	const double samplesPerTick = (sr * 60.0) / (bpm * 24.0);
	const juce::int64 target = (juce::int64)(transportSamplePos / samplesPerTick);

	// Resync ohne Ticks zu feuern: beim Play-Start, bei Rueckwaertsspruengen
	// (Loop/Seek) oder grossem Vorwaertssprung (Seek) den Zaehler einfach auf
	// die aktuelle Position setzen.
	const juce::int64 ahead = target - clockTicksFired;
	if (clockResyncPending.exchange(false) || ahead < 0 || ahead > 96)   // 96 = 1 Takt
	{
		clockTicksFired = target;
	}

	while (clockTicksFired < target)
	{
		if (editor->getModule()) {
			for (auto* mod : *editor->getModule()->getModules())
				sendMidiClockMessage(mod, activeChannel.load());
		}
		++clockTicksFired;
	}
}

void MidiProcessor::processMidiMessage(const juce::MidiMessage& msg)
{
	const int targetChannel = activeChannel.load();
	Track* const armedTrack = activeTrack.load();
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
				msg.getNoteNumber(), /*on=*/true, armedTrack);
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
				msg.getNoteNumber(), /*on=*/false, armedTrack);
		return; // fertig
	}

	//------------------------------------------------------------------
	// 3.  CONTROLLER / PITCH / CLOCK / START / STOP  
	//------------------------------------------------------------------
	if (msg.isController())
		for (auto* mod : *modules)
			sendControllerMessage(mod, targetChannel,
				msg.getControllerNumber(),
				msg.getControllerValue(),
				msg.getChannel(),
				armedTrack);

	else if (msg.isPitchWheel())
		for (auto* mod : *modules)
			sendPitchBendMessage(mod, targetChannel,
				msg.getPitchWheelValue(),
				msg.getChannel(),
				armedTrack);

	else if (msg.isMidiStart())
		for (auto* mod : *modules)
			sendMidiStartMessage(mod, targetChannel);

	else if (msg.isMidiStop())
		for (auto* mod : *modules)
			sendMidiStopMessage(mod, targetChannel);
}

Track* MidiProcessor::findTrackForModule(Module* m) {
	if (m == nullptr || editor == nullptr || editor->getModule() == nullptr)
		return nullptr;

	// A gate / plugin that belongs to a track is wired to that track's TrackIn
	// (see TrackUtil::createVirtualInstrumentTrack / createMIDITrack). NOTE:
	// those connections are stored with the endpoints reversed relative to the
	// data flow (e.g. source = TrackIn, target = gate), so the module can appear
	// as EITHER endpoint — check both. The TrackIn knows its Track.
	for (auto* c : *editor->getModule()->getConnections()) {
		if (c == nullptr) continue;

		Module* other = nullptr;
		if (c->source == m)      other = c->target;
		else if (c->target == m) other = c->source;
		else continue;

		if (auto* ti = dynamic_cast<TrackIn*>(other)) {
			if (Track* t = ti->getTrack())
				return t;
		}
	}
	return nullptr;
}

Track* MidiProcessor::resolveGateTrack(MidiGate* gate) {
	// Cached (positive) result, or set explicitly at track-creation time.
	if (gate->isTrackResolved())
		return gate->getTrack();

	Track* t = findTrackForModule(gate);
	if (t != nullptr)
		gate->setTrack(t);   // cache positive result; leave unresolved otherwise
	return t;
}

Track* MidiProcessor::resolvePluginTrack(PluginModule* plugin) {
	if (plugin->isTrackResolved())
		return plugin->getTrack();

	Track* t = findTrackForModule(plugin);
	if (t != nullptr)
		plugin->setTrack(t);
	return t;
}

void MidiProcessor::sendGateMessage(Module* module, int channel, int velocity, int note, bool on, Track* armedTrack) {
	if (auto* gate = dynamic_cast<MidiGate*>(module)) {
		Track* gateTrack = resolveGateTrack(gate);

		// Identity routing: a gate that belongs to a track fires only when that
		// track is the armed one — so two tracks can share a MIDI channel
		// without their plugins overlapping. Modular gates (no track), and
		// callers without an armed track (e.g. global all-notes-off), fall back
		// to classic channel matching.
		bool fire;
		if (armedTrack != nullptr && gateTrack != nullptr)
			fire = (gateTrack == armedTrack);
		else
			fire = (gate->getChannel() == channel);

		if (fire) {
			if (on && velocity > 0)
				gate->gateOn(velocity, note);
			else
				gate->gateOff(note);
		}
	}
	for (auto* sub : *module->getModules()) {
		sendGateMessage(sub, channel, velocity, note, on, armedTrack);
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

void MidiProcessor::sendPitchBendMessage(Module* module, int channel, int pitch, int midiInChannel, Track* armedTrack) {
	if (auto* pbm = dynamic_cast<PitchBendModule*>(module)) {
		// Modular pitch-bend module — broadcast as before (not track-bound).
		pbm->setPitch(pitch);
	}
	else if (auto* plugin = dynamic_cast<PluginModule*>(module)) {
		// Identity routing: pitch bend reaches a track instrument only when its
		// track is armed; modular plugins (no track) keep the broadcast.
		Track* pt = resolvePluginTrack(plugin);
		const bool deliver = (pt == nullptr) ? true : (pt == armedTrack);
		if (deliver)
			plugin->addMidiMessage(MidiMessage::pitchWheel(midiInChannel, pitch));
	}
	else if (auto* trackin = dynamic_cast<TrackIn*>(module)) {
		// Forward through the track chain (external MidiOut on MIDI tracks),
		// identity-gated so it doesn't bleed to same-channel tracks.
		Track* tt = trackin->getTrack();
		const bool deliver = (armedTrack == nullptr) || (tt == armedTrack);
		if (deliver) {
			Event* event = new Event("pitch", Event::Type::PITCH);
			event->setMessage(MidiMessage::pitchWheel(midiInChannel, pitch));
			event->setValue(static_cast<float>(pitch)); // MidiOut::handlePitchEvent reads getValue()
			trackin->eventReceived(event);
		}
	}
	for (auto* sub : *module->getModules()) {
		sendPitchBendMessage(sub, channel, pitch, midiInChannel, armedTrack);
	}
}

void MidiProcessor::sendControllerMessage(Module* module, int targetChannel, int controller, float value, int midiInChannel, Track* armedTrack) {

	if (auto* knob = dynamic_cast<Knob*>(module)) {
		// Knobs are modular MIDI-learn targets, matched by controller number and
		// not bound to a track — they keep seeing every CC.
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
		// Identity routing: a track instrument receives CC only when its track is
		// armed, so controllers don't bleed across same-channel instruments.
		// Modular plugins (no track) keep the classic broadcast.
		Track* pt = resolvePluginTrack(plugin);
		const bool deliver = (pt == nullptr) ? true : (pt == armedTrack);
		if (deliver)
			plugin->addMidiMessage(MidiMessage::controllerEvent(
				midiInChannel, controller, static_cast<int>(value)));
	}
	else if (auto* trackin = dynamic_cast<TrackIn*>(module)) {
		// Forward CC through the track chain (instrument plugin / external
		// MidiOut). Identity-gated so it doesn't bleed to same-channel tracks;
		// modular use (no armed track) keeps the previous broadcast behaviour.
		Track* tt = trackin->getTrack();
		const bool deliver = (armedTrack == nullptr) || (tt == armedTrack);
		if (deliver) {
			MidiMessage msg = MidiMessage::controllerEvent(
				midiInChannel, controller, static_cast<int>(value));
			Event* event = new Event("controller", Event::Type::CONTROLLER);
			event->setMessage(msg);
			trackin->eventReceived(event);
		}
	}
	for (auto* sub : *module->getModules()) {
		sendControllerMessage(sub, targetChannel, controller, value, midiInChannel, armedTrack);
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
		// Im Internal-Mode generiert MidiClock seine Ticks selbst auf einem
		// dedizierten HighResolutionTimer-Thread. Wenn wir hier auch noch
		// feuern, kommen Clocks doppelt raus (2x Tempo). Skip in dem Fall.
		if (midiClock->isInternal())
			return;
		midiClock->clock(MidiMessage::midiClock());
	}
}

void MidiProcessor::refreshMidiInputs() {
	auto inputs = MidiInput::getAvailableDevices();
	for (const auto& input : inputs) {
		if (deviceManager->isMidiInputDeviceEnabled(input.identifier)) {
			deviceManager->removeMidiInputDeviceCallback(input.identifier, this);
			deviceManager->addMidiInputDeviceCallback(input.identifier, this);
		}
	}
}

void MidiProcessor::enableAllMidiInputs() {
	auto inputs = MidiInput::getAvailableDevices();
	for (const auto& input : inputs) {
		if (deviceManager->isMidiInputDeviceEnabled(input.identifier)) {
			deviceManager->addMidiInputDeviceCallback(input.identifier, this);
		}
	}
}

void MidiProcessor::disableAllMidiInputs() {
	auto inputs = MidiInput::getAvailableDevices();
	for (const auto& input : inputs) {
		if (deviceManager->isMidiInputDeviceEnabled(input.identifier)) {
			deviceManager->removeMidiInputDeviceCallback(input.identifier, this);
		}
	}
}

void MidiProcessor::startMidiClock()
{
	clockRunning = true;
	// Tick-Zaehler beim naechsten timerCallback an die aktuelle Playhead-
	// Position koppeln (kein Backfill).
	clockResyncPending = true;
}

void MidiProcessor::stopMidiClock()
{
	clockRunning = false;
}

void MidiProcessor::setMidiClockBPM(double bpm)
{
	// Tempo kommt zur Laufzeit aus Project::getTempo() (die Clock leitet ihre
	// Ticks aus der Sample-Position ab). clockBPM bleibt nur als zuletzt
	// gesetzter Wert erhalten — die eigentliche Tick-Berechnung nutzt ihn nicht.
	clockBPM = bpm;
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

