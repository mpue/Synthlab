/*
  ==============================================================================

    TrackUtil.h
    Created: 8 Jul 2025 10:35:03am
    Author:  mpue

  ==============================================================================
*/

#include "Tracks/Track.h"
#include "Actions/AddModuleAction.h"
#include "MidiGate.h"
#include "TrackIn.h"
#include "AudioIn.h"
#include "AuxOut.h"
#include "MidiOut.h"
#include "SynthEditor.h"
#include "Tracks/TrackNavigator.h"
#include "PluginModule.h"
#include "Project/Project.h"
#include "PropertyView.h"
#include "Settings.h"

#pragma once
class TrackUtil
{
public:
	/**
	 * Creates a new audio track with the given name and position.
	 * @param name the name of the track
	 * @param pos the position of the track
	 * @param navigator the track navigator to add the track to
	 * @param editor the editor to which the track belongs
	 * @param module the module to which the connections will be added
	 * @return a pointer to the created Track object
	 */
	static Track* createVirtualInstrumentTrack(juce::String name, Point<int> pos, TrackNavigator* navigator,SynthEditor* editor)
	{
		int channel = getNextFreeMidiChannel(navigator);

		AddModuleAction* am = new AddModuleAction(editor, pos, 61);
		Project::getInstance()->getUndoManager()->beginNewTransaction();
		Project::getInstance()->getUndoManager()->perform(am);
		MidiGate* gate = dynamic_cast<MidiGate*>(am->getModule());
        if (gate == nullptr) return nullptr;
		gate->setChannel(channel);

		pos.x += 160;

		AddModuleAction* am1 = new AddModuleAction(editor, pos, 48);
		Project::getInstance()->getUndoManager()->beginNewTransaction();
		Project::getInstance()->getUndoManager()->perform(am1);
		TrackIn* ti = dynamic_cast<TrackIn*>(am1->getModule());

        if (ti == nullptr) return nullptr;
        
		Track* t = navigator->addTrack(Track::Type::INSTRUMENT, Project::getInstance()->getSampleRate());
		t->setType(Track::Type::INSTRUMENT);
		t->setInstrumentName(name);

		// Track-Name = Plugin-Name. Der Channel-Strip uebernimmt das ueber
		// MasterChannelPanel::setTrack -> setName, und der PropertyView ueber
		// den NameValue-Seed in Track::createProperties.
		t->setName(name);
		ti->setTrack(t);
		gate->setTrack(t);   // identity routing: live input follows this track
		t->setVolume(1.0f);
		t->setMidiChannel(channel, false);

		Connection* c1 = new Connection(ti, ti->pins.at(4), gate, gate->pins.at(1));
		editor->getModule()->addConnection(c1);		
		gate->connectPins(1, ti->pins.at(4));

		pos.x += 160;

		AddModuleAction* am2 = new AddModuleAction(editor, pos, 79);
		Project::getInstance()->getUndoManager()->beginNewTransaction();
		Project::getInstance()->getUndoManager()->perform(am2);
		PluginModule* pm = dynamic_cast<PluginModule*>(am2->getModule());
        
        if (pm == nullptr) return nullptr;
        
		pm->selectPlugin(name);
		pm->setChannel(channel);
		pm->setTrack(t);   // identity routing: live CC follows this track

		Connection* c2 = new Connection(pm, pm->pins.at(4), ti, ti->pins.at(5));
		editor->getModule()->addConnection(c2);		
		ti->connectPins(5, pm->pins.at(4));

		// connect the audio out — module type controlled by Settings.
		// AudioOut (default): wire the plugin to the master AudioOut (or AuxOut
		// fallback via ConnectAudioOut when the master is already taken).
		// AuxOut: always create a fresh AuxOut so each instrument lands on its
		// own mixer strip without sharing the master out.

		pos.x += 160;

		t->setPluginModule(pm);

		const bool preferAuxOutInstr =
			Settings::getInstance()->getNewTrackOutputType()
				== Settings::NewTrackOutputType::AuxOut;

		Module* destInstr = nullptr;

		if (preferAuxOutInstr) {
			AddModuleAction* am3 = new AddModuleAction(editor, pos, 77);
			Project::getInstance()->getUndoManager()->beginNewTransaction();
			Project::getInstance()->getUndoManager()->perform(am3);
			AuxOut* auxOut = dynamic_cast<AuxOut*>(am3->getModule());

			if (auxOut != nullptr) {
				auxOut->pins.at(0)->getConnections().push_back(pm->pins.at(2));
				auxOut->pins.at(1)->getConnections().push_back(pm->pins.at(3));
				Connection* c1 = new Connection(pm, pm->pins.at(2), auxOut, auxOut->pins.at(0));
				Connection* c2 = new Connection(pm, pm->pins.at(3), auxOut, auxOut->pins.at(1));
				editor->getModule()->getConnections()->push_back(c1);
				editor->getModule()->getConnections()->push_back(c2);
				destInstr = auxOut;
			}
		}
		else {
			AddModuleAction* am3 = new AddModuleAction(editor, pos, 66);
			Project::getInstance()->getUndoManager()->beginNewTransaction();
			Project::getInstance()->getUndoManager()->perform(am3);
			AudioOut* audioOut = dynamic_cast<AudioOut*>(am3->getModule());

            if (audioOut == nullptr) return nullptr;
            
			destInstr = ConnectAudioOut(audioOut, pm, pos, 2, 3, editor->getModule(), editor);
		}

		// Wire the Track to its mixer channel panel so the strip shows the
		// instrument name — works for either AudioOut or AuxOut destinations.
		LinkTrackToMixerPanel(t, destInstr, editor);

		// SynthEditor::resized() rechnet pro Connection die Endpunkte aus und
		// laesst Connection::resized den Bezier-Pfad bauen. Ohne diesen Trigger
		// stehen p1/p2/linePath beim ersten Paint auf den Defaults und die
		// frisch verlegten Verbindungen sind unsichtbar, bis der User ein Modul
		// bewegt (was JUCE-intern ein resized() auf dem Parent ausloest).
		if (editor != nullptr) editor->resized();

		// Draw the user's eye to the "Show Plugin" affordance for the freshly
		// created instrument track. Only fires from this user-driven path —
		// project-load (SynthEditor::setConfiguration) builds instrument
		// tracks via navigator->addTrack directly and stays quiet.
		if (auto* pv = Project::getInstance()->getPropertyView())
			pv->pulseShowPluginButton (t);

		return t;
	}

	// Liefert das tatsaechliche Ziel-Modul zurueck (AudioOut, falls direkt
	// verkabelt — sonst der frisch angelegte AuxOut). Wird vom Caller
	// gebraucht, um den zugehoerigen MasterChannelPanel-Strip zu finden.
	static Module* ConnectAudioOut(AudioOut*& audioOut, Module* m, const juce::Point<int>& pos, int leftSourcePin, int rightSourcePin, Module* module, SynthEditor* editor)
	{
		if (audioOut != nullptr) {
			// audio out is not connected, thus make some connections
			if (audioOut->pins.at(0)->getConnections().size() == 0 && audioOut->pins.at(1)->getConnections().size() == 0) {
				audioOut->connectPins(0, m->pins.at(leftSourcePin));
				audioOut->connectPins(1, m->pins.at(rightSourcePin));
				Connection* c1 = new Connection(m, m->pins.at(2), audioOut, audioOut->pins.at(0));
				Connection* c2 = new Connection(m, m->pins.at(3), audioOut, audioOut->pins.at(1));
				module->addConnection(c1);
				module->addConnection(c2);
				audioOut->setVolume(1.0f);
				return audioOut;
			}
			// already connected, create an aux out
			else {
				AddModuleAction* am = new AddModuleAction(editor, pos, 77);
				Project::getInstance()->getUndoManager()->beginNewTransaction();
				Project::getInstance()->getUndoManager()->perform(am);
				AuxOut* ao = dynamic_cast<AuxOut*>(am->getModule());
				if (ao != nullptr) {
					ao->pins.at(0)->getConnections().push_back(m->pins.at(leftSourcePin));
					ao->pins.at(1)->getConnections().push_back(m->pins.at(rightSourcePin));
					Connection* c1 = new Connection(m, m->pins.at(leftSourcePin), ao, ao->pins.at(0));
					Connection* c2 = new Connection(m, m->pins.at(rightSourcePin), ao, ao->pins.at(1));
					module->getConnections()->push_back(c1);
					module->getConnections()->push_back(c2);
					return ao;
				}

			}
		}
		return nullptr;
	}

	// Sucht den MasterChannelPanel zu einem AudioOut/AuxOut und linked ihn
	// an den Track, damit Channel-Strip-FX automatisierbar werden
	// (Track::fxPlugins[] wird nur ueber MasterChannelPanel::setTrack ->
	// syncTrackFXPlugins befuellt).
	static void LinkTrackToMixerPanel(Track* t, Module* destination, SynthEditor* editor)
	{
		if (t == nullptr || destination == nullptr || editor == nullptr) return;
		auto mixer = editor->getMixer();
		if (mixer == nullptr) return;

		int chIdx = -1;
		if (auto* ao = dynamic_cast<AudioOut*>(destination)) chIdx = ao->getChannelIndex();
		else if (auto* ax = dynamic_cast<AuxOut*>(destination)) chIdx = ax->getChannelIndex();
		if (chIdx < 0) return;

		for (auto* panel : mixer->getChannels())
		{
			if (panel != nullptr && panel->getIndex() == chIdx)
			{
				panel->setTrack(t);
				break;
			}
		}
	}

	// `name`: optional track name. When non-empty it is set BEFORE the track is
	// linked to its mixer channel strip (LinkTrackToMixerPanel below), so the
	// strip shows it immediately. Renaming the track *after* this function
	// returns would only update the navigator — the mixer strip took its label
	// from the track name at link time and a late rename does not reliably
	// propagate. Default: the generic "Audio N" name.
	static Track* createAudioTrack(SynthEditor* editor, TrackNavigator* navigator, Module* module, const juce::String& name = juce::String())
	{
		// Count existing audio tracks to determine the next available hardware input pair.
		// Track N gets inputs (N*2, N*2+1).
		int audioTrackCount = 0;
		for (auto* track : navigator->getTracks()) {
			if (track->getType() == Track::Type::AUDIO)
				audioTrackCount++;
		}
		int leftInputChannel  = audioTrackCount * 2;
		int rightInputChannel = audioTrackCount * 2 + 1;

		// Create AudioIn L module. UndoManager::perform takes ownership and
		// DELETES the action when perform() returns false (e.g.
		// AddModuleAction bails because numCurrentInputs >= numActiveInputs).
		// Touching the pointer afterwards is a use-after-free — always check
		// the bool return before dereferencing the action.
		Point<int> origin = editor->getNextTrackStartPosition();
		Point<int> pos = origin;
		auto* um = Project::getInstance()->getUndoManager();

		AudioIn* audioInL = nullptr;
		{
			AddModuleAction* amAL = new AddModuleAction(editor, pos, 76);
			um->beginNewTransaction();
			if (um->perform(amAL)) {
				audioInL = dynamic_cast<AudioIn*>(amAL->getModule());
				if (audioInL != nullptr)
					audioInL->setSelectedAudioInput(leftInputChannel);
			}
			// If perform returned false, amAL has been deleted — don't touch.
		}

		// Create AudioIn R module below L
		pos.y += 155;
		AudioIn* audioInR = nullptr;
		{
			AddModuleAction* amAR = new AddModuleAction(editor, pos, 76);
			um->beginNewTransaction();
			if (um->perform(amAR)) {
				audioInR = dynamic_cast<AudioIn*>(amAR->getModule());
				if (audioInR != nullptr)
					audioInR->setSelectedAudioInput(rightInputChannel);
			}
		}

		// Create TrackIn module to the right of the AudioIn pair
		pos = Point<int>(origin.x + 170, origin.y + 70);
		TrackIn* ti = nullptr;
		{
			AddModuleAction* am = new AddModuleAction(editor, pos, 48);
			um->beginNewTransaction();
			if (um->perform(am)) {
				ti = dynamic_cast<TrackIn*>(am->getModule());
			}
		}

        if (ti == nullptr) return nullptr;
        
		Track* t = navigator->addTrack(Track::Type::AUDIO, Project::getInstance()->getSampleRate());
		t->setName(name.isNotEmpty() ? name
		                             : ("Audio " + String(navigator->getTracks().size() + 1)));
		ti->setTrack(t);

		// Wire AudioIn L OUT (pin 0) → TrackIn IN L (pin 0)
		if (audioInL != nullptr) {
			ti->connectPins(0, audioInL->pins.at(0));
			Connection* cL = new Connection(audioInL, audioInL->pins.at(0), ti, ti->pins.at(0));
			module->addConnection(cL);
		}

		// Wire AudioIn R OUT (pin 0) → TrackIn IN R (pin 1)
		if (audioInR != nullptr) {
			ti->connectPins(1, audioInR->pins.at(0));
			Connection* cR = new Connection(audioInR, audioInR->pins.at(0), ti, ti->pins.at(1));
			module->addConnection(cR);
		}

		// Output module — type controlled by Settings (AudioOut vs. AuxOut).
		// AudioOut path: re-use the first existing AudioOut if it has no
		// connections, otherwise spawn an AuxOut (ConnectAudioOut handles that).
		// AuxOut path: always spawn a fresh AuxOut, so the track lands on its
		// own mixer strip without taking over the master out.
		pos = Point<int>(origin.x + 330, origin.y + 70);
		Module* dest = nullptr;

		const bool preferAuxOut =
			Settings::getInstance()->getNewTrackOutputType()
				== Settings::NewTrackOutputType::AuxOut;

		if (preferAuxOut) {
			AddModuleAction* am3 = new AddModuleAction(editor, pos, 77);
			um->beginNewTransaction();
			AuxOut* auxOut = nullptr;
			if (um->perform(am3))
				auxOut = dynamic_cast<AuxOut*>(am3->getModule());
			// am3 deleted by UndoManager if perform() returned false.

			if (auxOut != nullptr) {
				auxOut->pins.at(0)->getConnections().push_back(ti->pins.at(2));
				auxOut->pins.at(1)->getConnections().push_back(ti->pins.at(3));
				Connection* c1 = new Connection(ti, ti->pins.at(2), auxOut, auxOut->pins.at(0));
				Connection* c2 = new Connection(ti, ti->pins.at(3), auxOut, auxOut->pins.at(1));
				module->getConnections()->push_back(c1);
				module->getConnections()->push_back(c2);
				dest = auxOut;
			}
		}
		else {
			AudioOut* audioOut = nullptr;

			for (int i = 0; i < editor->getModule()->getModules()->size(); i++) {
				if (editor->getModule()->getModules()->at(i) != nullptr) {
					audioOut = dynamic_cast<AudioOut*>(editor->getModule()->getModules()->at(i));
					if (audioOut != nullptr) {
						break;
					}
				}
			}

			if (audioOut == nullptr) {
				AddModuleAction* am3 = new AddModuleAction(editor, pos, 66);
				um->beginNewTransaction();
				if (um->perform(am3))
					audioOut = dynamic_cast<AudioOut*>(am3->getModule());
				// am3 deleted by UndoManager if perform() returned false.
			}

			dest = ConnectAudioOut(audioOut, ti, pos, 2, 3, module, editor);
		}

		// Channel-Strip an den Track binden, sonst bleibt MasterChannelPanel::track
		// nullptr und syncTrackFXPlugins() liefert keine FX-Plugins zurueck —
		// Folge: Plugin-Combo im TrackControl ist leer, Automation unmoeglich.
		LinkTrackToMixerPanel(t, dest, editor);

		// Erzwingt das Aufrechnen der Connection-Endpunkte; ohne resized() sind
		// die neuen Verbindungen bis zum ersten Modul-Drag unsichtbar.
		if (editor != nullptr) editor->resized();
		return t;
	}

	static Track* createMIDITrack(SynthEditor* editor, TrackNavigator* navigator, Module* module)
	{
		int channel = getNextFreeMidiChannel(navigator);

		Point<int> pos = editor->getNextTrackStartPosition();

		AddModuleAction* am = new AddModuleAction(editor, pos, 61);
		Project::getInstance()->getUndoManager()->beginNewTransaction();
		Project::getInstance()->getUndoManager()->perform(am);
		MidiGate* gate = dynamic_cast<MidiGate*>(am->getModule());

		pos.x += 160;
		AddModuleAction* am1 = new AddModuleAction(editor, pos, 48);
		Project::getInstance()->getUndoManager()->beginNewTransaction();
		Project::getInstance()->getUndoManager()->perform(am1);
		TrackIn* ti = dynamic_cast<TrackIn*>(am1->getModule());

		Track* t = navigator->addTrack(Track::Type::MIDI, Project::getInstance()->getSampleRate());
		t->setName("MIDI " + String(navigator->getTracks().size() + 1));
		ti->setTrack(t);
		gate->setTrack(t);   // identity routing: live input follows this track

		Connection* c1 = new Connection(ti, ti->pins.at(4), gate, gate->pins.at(1));
		module->getConnections()->push_back(c1);
		gate->pins.at(1)->getConnections().push_back(ti->pins.at(4));
				
		pos.x += 160;
		AddModuleAction* am2 = new AddModuleAction(editor, pos, 62); // MidiOut prefab ID is 62
		Project::getInstance()->getUndoManager()->beginNewTransaction();
		Project::getInstance()->getUndoManager()->perform(am2);
		MidiOut* midiOut = dynamic_cast<MidiOut*>(am2->getModule());

		if (midiOut) {
			// Set the MIDI channel for the MidiOut to match the track
			t->setMidiChannel(channel, false);
			midiOut->setMidiChannel(channel);

			// Connect TrackIn's EVENT output pin (index 5) to MidiOut's EVENT input pin (index 0)
			Connection* c2 = new Connection(midiOut, midiOut->pins.at(0), ti, ti->pins.at(5));
			module->getConnections()->push_back(c2);
			ti->pins.at(5)->getConnections().push_back(midiOut->pins.at(0));
		}

		Array<var> values = Array<var>();

		for (int i = 0; i < PluginManager::getInstance()->getPlugins().size(); i++) {
			values.add(PluginManager::getInstance()->getPlugins().getReference(i));
		}

		// Connection-Endpunkte aufrechnen, sonst sind die frischen Verbindungen
		// im Editor erst nach dem ersten Modul-Drag sichtbar.
		if (editor != nullptr) editor->resized();
		return t;
	}

	static int getNextFreeMidiChannel(TrackNavigator* navigator) {
		std::set<int> usedChannels;
		std::vector<Track*> tracks = navigator->getTracks();

		int channelTrackCount = 0;

		// Sammle alle verwendeten MIDI-Kanäle
		for (const auto& track : tracks) {
			if (track->getType() == Track::Type::MIDI || track->getType() == Track::Type::INSTRUMENT) {
				usedChannels.insert(track->getMidiChannel());
				++channelTrackCount;
			}
		}

		// Finde den ersten freien Kanal (normalerweise 1-16 für MIDI)
		for (int channel = 1; channel <= 16; channel++) {
			if (usedChannels.find(channel) == usedChannels.end()) {
				return channel;
			}
		}

		// Alle 16 Kanäle belegt: Unter identitätsbasiertem Routing (siehe
		// MidiProcessor) brauchen Instrument-Tracks keinen eindeutigen Kanal
		// mehr, also verteilen wir weitere Tracks per Round-Robin, statt alles
		// auf Kanal 1 zu stapeln.
		return (channelTrackCount % 16) + 1;
	}


};
