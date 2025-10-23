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
#include "MidiOut.h"
#include "SynthEditor.h"
#include "Tracks/TrackNavigator.h"
#include "PluginModule.h"

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
		gate->setChannel(channel);

		pos.x += 160;

		AddModuleAction* am1 = new AddModuleAction(editor, pos, 48);
		Project::getInstance()->getUndoManager()->beginNewTransaction();
		Project::getInstance()->getUndoManager()->perform(am1);
		TrackIn* ti = dynamic_cast<TrackIn*>(am1->getModule());

		Track* t = navigator->addTrack(Track::Type::INSTRUMENT, Project::getInstance()->getSampleRate());
		t->setType(Track::Type::INSTRUMENT);
		t->setInstrumentName(name);

		// Set the name of the track based on the number of existing tracks
		t->setName("MIDI " + String(navigator->getTracks().size() + 1));
		ti->setTrack(t);
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
		pm->selectPlugin(name);
		pm->setChannel(channel);

		Connection* c2 = new Connection(pm, pm->pins.at(4), ti, ti->pins.at(5));
		editor->getModule()->addConnection(c2);		
		ti->connectPins(5, pm->pins.at(4));

		// connect the audio out
		
		pos.x += 160;

		AudioOut* audioOut = nullptr;

		AddModuleAction* am3 = new AddModuleAction(editor, pos, 66);
		Project::getInstance()->getUndoManager()->beginNewTransaction();
		Project::getInstance()->getUndoManager()->perform(am3);
		audioOut = dynamic_cast<AudioOut*>(am3->getModule());
			
		t->setPluginModule(pm);

		AuxOut* auxOut = nullptr;


		ConnectAudioOut(audioOut, pm, pos, 2, 3, editor->getModule(), editor);
		return t;
	}

	static void ConnectAudioOut(AudioOut*& audioOut, Module* m, const juce::Point<int>& pos, int leftSourcePin, int rightSourcePin, Module* module, SynthEditor* editor)
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
				}

			}
		}

	}

	static Track* createAudioTrack(SynthEditor* editor, TrackNavigator* navigator, Module* module)
	{
		Point<int> pos = Point<int>(10, 10);
		AddModuleAction* am = new AddModuleAction(editor, pos, 48);
		Project::getInstance()->getUndoManager()->beginNewTransaction();
		Project::getInstance()->getUndoManager()->perform(am);

		TrackIn* ti = dynamic_cast<TrackIn*>(am->getModule());

		Track* t = navigator->addTrack(Track::Type::AUDIO, Project::getInstance()->getSampleRate());
		t->setName("Audio " + String(navigator->getTracks().size() + 1));
		ti->setTrack(t);

		Module* m = module;

		pos.x += 160;

		AuxOut* auxOut = nullptr;
		AudioIn* audioIn = nullptr;

		// find audio in
		for (int i = 0; i < m->getModules()->size(); i++) {
			if (m->getModules()->at(i) != nullptr) {
				audioIn = dynamic_cast<AudioIn*>(m->getModules()->at(i));
				if (audioIn != nullptr) {
					break;
				}
			}
		}
		//  find aux out
		for (int i = 0; i < m->getModules()->size(); i++) {
			if (m->getModules()->at(i) != nullptr) {
				auxOut = dynamic_cast<AuxOut*>(m->getModules()->at(i));
				if (auxOut != nullptr) {
					break;
				}
			}
		}

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
			// no audio out found, create one
			AddModuleAction* am3 = new AddModuleAction(editor, pos, 66);
			Project::getInstance()->getUndoManager()->beginNewTransaction();
			Project::getInstance()->getUndoManager()->perform(am3);
			audioOut = dynamic_cast<AudioOut*>(am3->getModule());
		}

		// found one
		ConnectAudioOut(audioOut, ti, pos, 2, 3,module, editor);
		return t;
	}

	static Track* createMIDITrack(SynthEditor* editor, TrackNavigator* navigator, Module* module)
	{
		int channel = getNextFreeMidiChannel(navigator);

		Point<int> pos = Point<int>(10, 10);

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

		return t;
	}

	static int getNextFreeMidiChannel(TrackNavigator* navigator) {
		std::set<int> usedChannels;
		std::vector<Track*> tracks = navigator->getTracks();

		// Sammle alle verwendeten MIDI-Kanäle
		for (const auto& track : tracks) {
			if (track->getType() == Track::Type::MIDI || track->getType() == Track::Type::INSTRUMENT) {
				usedChannels.insert(track->getMidiChannel());
			}
		}

		// Finde den ersten freien Kanal (normalerweise 1-16 für MIDI)
		for (int channel = 1; channel <= 16; channel++) {
			if (usedChannels.find(channel) == usedChannels.end()) {
				return channel;
			}
		}

		// Falls alle Kanäle belegt sind, gib den ersten zurück (oder handle den Fehlerfall)
		return 1;
	}


};