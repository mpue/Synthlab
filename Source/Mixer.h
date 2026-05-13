//
//  Mixer.hpp
//  Synthlab - App
//
//  Created by Matthias Pueski on 23.04.18.
//  Copyright © 2018 Pueski. All rights reserved.
//

#ifndef Mixer_h
#define Mixer_h

#include "../JuceLibraryCode/JuceHeader.h"
#include "Tracks/Track.h"
#include <algorithm>

class Mixer : public ChangeListener, public ChangeBroadcaster {

public:

	static Mixer* getInstance() {
		if (instance == NULL) {
			instance = new Mixer();
		}
		return instance;
	}

	static void destroy() {
		delete instance;
	}

	class Channel {

	public:
		enum Type {
			IN,
			OUT,
			AUX,
			MASTER,
			BUS
		};

		// Defaults: unity gain, center pan. Ohne diese Inits blieben volume/pan
		// auf dem Heap-Default haengen (bei new Mixer::Channel() = 0/0) bis
		// jemand den Slider beruehrt — Folge: frisch angelegte OUT/AUX/IN-Strips
		// waren still, auch wenn der Audio-Pfad oben drueber funktioniert
		// (TrackIn schreibt mit Track-Volume, der Mix-Loop multipliziert
		// hinterher nochmal mit channel->volume == 0). Im Bounce besonders
		// auffaellig, weil dort nichts die Slider zwischenzeitlich anfasst.
		double volume = 1.0;
		double pan    = 0.0;
		float magnitudeLeft = 0;
		float magnitudeRight = 0;
		bool mute = false;
		bool solo = false;
		juce::String name;
		Type channelType;
		int index;
	};


	inline void addTrack(Track* track) {
		this->tracks.push_back(track);
		track->addChangeListener(this);
		sendChangeMessage();
	}

	inline void removeTrack(Track* track) {
		track->removeChangeListener(this);
		tracks.erase(std::remove(tracks.begin(), tracks.end(), track), tracks.end());
		// lastModified zeigt sonst auf den gerade weggeraeumten Track — der
		// Mixer wird via changeListenerCallback bei jeder Track-Aenderung neu
		// gesetzt, aber bis dahin ist der Pointer dangling und getLastModifiedTrack()
		// dispatcht in Freigegebenes.
		if (lastModified == track)
			lastModified = nullptr;
		sendChangeMessage();
	}

	vector<Track*> getTracks() {
		return this->tracks;
	}

	// Liefert true, wenn mind. ein Track Solo geschaltet ist. Wird vom
	// TrackIn::process gelesen, um nicht-solo-getrackte Spuren stumm zu
	// schalten — lineare Suche, aber n=#tracks ist klein und der Ruf liegt
	// nicht im inneren Loop.
	inline bool anyTrackSoloed() {
		for (auto* t : tracks)
			if (t != nullptr && t->isSolo())
				return true;
		return false;
	}

	inline virtual void changeListenerCallback(ChangeBroadcaster* source) override {

		if (Track* t = dynamic_cast<Track*>(source)) {
			lastModified = t;
			sendChangeMessage();
		}

	}

	Track* getLastModifiedTrack() {
		return this->lastModified;
	}

	inline void addChangeListener(ChangeListener* listener) {
		ChangeBroadcaster::addChangeListener(listener);
	}

	std::vector<Channel*>& getChannels();
	void addChannel(Channel* channel);
	void removeChannel(int index);
	void removeAllChannels();

	Channel* getMasterChannel() { return masterChannel; }

	Channel* getChannel(Mixer::Channel::Type type, int index);

	// Realtime-safe O(1) accessor for the audio thread. Maintained as a
	// by-type cache rebuilt on every add/remove. Read-only — do not mutate
	// while the audio thread might be iterating.
	inline const std::vector<Channel*>& getChannelsByType(Channel::Type t) const {
		return byType[(int)t];
	}

	int getNumOutputs();
	int getNumInputs();
	int getNumAuxBusses();
	int getNumBusChannels();

	void setAvailableInputChannelNames(StringArray channels) {
		this->availableInputChannels = channels;
	}

	StringArray getInputChannels() {
		return this->availableInputChannels;
	}

	void setAvailableOutputChannelNames(StringArray channels) {
		this->availableOutputChannels = channels;
	}

	StringArray getOutputChannels() {
		return this->availableOutputChannels;
	}

	vector<String> getMidiInputs() {
		return midiInputs;
	}

	vector<String> getMidiOutputs() {
		return midiOutputs;
	}

	void clearMidiInputs() {
		this->midiInputs.clear();
	}

	void clearMidiOutputs() {
		this->midiOutputs.clear();
	}

	void addMidiInput(String name) {
		this->midiInputs.push_back(name);
	}

	void addMidiOutput(String name) {
		this->midiOutputs.push_back(name);
	}

private:

	Mixer();
	~Mixer();

	static Mixer* instance;

	Channel* masterChannel = nullptr;

	vector<Track*> tracks;
	Track* lastModified = NULL;

	std::vector<Channel*> channels;
	// Index-by-type for O(1) audio-thread lookups (F1). Rebuilt by rebuildByType().
	// Slot count must match the Type enum (IN, OUT, AUX, MASTER, BUS).
	static constexpr int kNumChannelTypes = 5;
	std::vector<Channel*> byType[kNumChannelTypes];
	void rebuildByType();

	StringArray availableInputChannels;
	StringArray availableOutputChannels;

	vector<String> midiInputs;
	vector<String> midiOutputs;
};

#endif /* Mixer_hpp */
