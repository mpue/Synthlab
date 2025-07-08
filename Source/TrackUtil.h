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

		Track* t = navigator->addTrack(Track::Type::INSTRUMENT, 48000);
		t->setType(Track::Type::INSTRUMENT);
		t->setInstrumentName(name);

		// Set the name of the track based on the number of existing tracks
		t->setName("MIDI " + String(navigator->getTracks().size() + 1));
		ti->setTrack(t);
		t->setVolume(1.0f);
		t->setMidiChannel(channel, false);

		Connection* c1 = new Connection(ti, ti->pins.at(4), gate, gate->pins.at(1));
		editor->getModule()->getConnections()->push_back(c1);
		gate->pins.at(1)->getConnections().push_back(ti->pins.at(4));

		pos.x += 160;

		AddModuleAction* am2 = new AddModuleAction(editor, pos, 79);
		Project::getInstance()->getUndoManager()->beginNewTransaction();
		Project::getInstance()->getUndoManager()->perform(am2);
		PluginModule* pm = dynamic_cast<PluginModule*>(am2->getModule());
		pm->selectPlugin(name);

		Connection* c2 = new Connection(pm, pm->pins.at(4), ti, ti->pins.at(5));
		editor->getModule()->getConnections()->push_back(c2);
		ti->pins.at(5)->getConnections().push_back(pm->pins.at(4));

		AuxOut* audioOut = nullptr;

		pos.x += 160;

		ConnectAudioOut(audioOut, pm, pos, 2, 3, editor->getModule(), editor);
		return t;
	}

	static void ConnectAudioOut(AuxOut*& audioOut, Module* m, const juce::Point<int>& pos, int leftSourcePin, int rightSourcePin, Module* module, SynthEditor* editor)
	{
		if (audioOut != nullptr) {
			// audio out is not connected, thus make some connections
			if (audioOut->pins.at(0)->getConnections().size() == 0 && audioOut->pins.at(1)->getConnections().size() == 0) {
				audioOut->pins.at(0)->getConnections().push_back(m->pins.at(leftSourcePin));
				audioOut->pins.at(1)->getConnections().push_back(m->pins.at(rightSourcePin));
				Connection* c1 = new Connection(m, m->pins.at(0), audioOut, audioOut->pins.at(0));
				Connection* c2 = new Connection(m, m->pins.at(1), audioOut, audioOut->pins.at(1));
				module->getConnections()->push_back(c1);
				module->getConnections()->push_back(c2);
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
		// no audio out, create one
		else {

			AddModuleAction* am = new AddModuleAction(editor, pos, 77);
			Project::getInstance()->getUndoManager()->beginNewTransaction();
			Project::getInstance()->getUndoManager()->perform(am);
			audioOut = dynamic_cast<AuxOut*>(am->getModule());

			if (audioOut != nullptr) {
				audioOut->pins.at(0)->getConnections().push_back(m->pins.at(leftSourcePin));
				audioOut->pins.at(1)->getConnections().push_back(m->pins.at(rightSourcePin));
				Connection* c1 = new Connection(m, m->pins.at(leftSourcePin), audioOut, audioOut->pins.at(0));
				Connection* c2 = new Connection(m, m->pins.at(rightSourcePin), audioOut, audioOut->pins.at(1));
				module->getConnections()->push_back(c1);
				module->getConnections()->push_back(c2);
				audioOut->setVolume(1.0f);
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

		Track* t = navigator->addTrack(Track::Type::AUDIO, 48000);
		t->setName("Audio " + String(navigator->getTracks().size() + 1));
		ti->setTrack(t);

		Module* m = module;

		pos.x += 160;

		AuxOut* audioOut = nullptr;
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
		//  find audio out
		for (int i = 0; i < m->getModules()->size(); i++) {
			if (m->getModules()->at(i) != nullptr) {
				audioOut = dynamic_cast<AuxOut*>(m->getModules()->at(i));
				if (audioOut != nullptr) {
					break;
				}
			}
		}
		// found one
		ConnectAudioOut(audioOut, ti, pos, 0, 1,module, editor);
		return t;
	}

	static Track* createMIDITrack(SynthEditor* editor, TrackNavigator* navigator, Module* module)
	{
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

		Track* t = navigator->addTrack(Track::Type::MIDI, 48000);
		t->setName("MIDI " + String(navigator->getTracks().size() + 1));
		ti->setTrack(t);

		Connection* c1 = new Connection(ti, ti->pins.at(4), gate, gate->pins.at(1));
		module->getConnections()->push_back(c1);
		gate->pins.at(1)->getConnections().push_back(ti->pins.at(4));

		Array<var> values = Array<var>();

		for (int i = 0; i < PluginManager::getInstance()->getPlugins().size(); i++) {
			values.add(PluginManager::getInstance()->getPlugins().getReference(i));
		}

		return t;
	}

	static int getNextFreeMidiChannel(TrackNavigator* navigator) {
		int channel = 1;
		std::vector<Track*> tracks = navigator->getTracks();
		for (int i = 0; i < tracks.size(); i++) {
			if (tracks[i]->getType() == Track::Type::MIDI) {
				if (tracks[i]->getMidiChannel() == channel) {
					channel++;
				}
			}
		}
		return channel;
	}


};