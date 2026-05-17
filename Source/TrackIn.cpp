	/*
	 ==============================================================================

	 MidiGate.cpp
	 Created: 4 Apr 2018 3:48:23pm
	 Author:  Matthias Pueski

	 ==============================================================================
	 */

	#include "../JuceLibraryCode/JuceHeader.h"
	#include "TrackIn.h"
	#include "Connection.h"
	#include "Mixer.h"

	#define M_PI 3.14159265358979323846


	using juce::Justification;
	using juce::ImageCache;

	//==============================================================================
	TrackIn::TrackIn(Track::Type type,TrackNavigator* navigator, double sampleRate, int buffersize)
	{
		this->trackType = type;
		setSize(120, 140);

		this->sampleRate = sampleRate;
		this->buffersize = buffersize;

		setName("Track In");

		editable = false;
		prefab = true;
		this->navigator = navigator;		
		createProperties();
	}

	TrackIn::~TrackIn()
	{
		delete volumeValue;
		delete volumeListener;
		delete trackValue;
		delete trackListener;

	}

	void TrackIn::createProperties() {
		volumeValue = new Value();
		volumeListener = new ValueListener(*volumeValue, this);
		trackValue = new Value();
		trackListener = new TrackListener(*trackValue, this);
	}

	void TrackIn::changeListenerCallback(ChangeBroadcaster* source)
	{
		//Track* track = dynamic_cast<Track*>(source);
		//if (track != nullptr) {
		//	offset = track->getSamplePosition();
		//}

	}

	void TrackIn::configurePins() {
		Pin* p1 = new Pin(Pin::Type::AUDIO);
		p1->direction = Pin::Direction::IN;
		p1->setName("L");

		Pin* p2 = new Pin(Pin::Type::AUDIO);
		p2->direction = Pin::Direction::IN;
		p2->setName("R");

		Pin* p3 = new Pin(Pin::Type::AUDIO);
		p3->direction = Pin::Direction::OUT;
		p3->setName("L");

		Pin* p4 = new Pin(Pin::Type::AUDIO);
		p4->direction = Pin::Direction::OUT;
		p4->setName("R");

		Pin* p5 = new Pin(Pin::Type::EVENT);
		p5->direction = Pin::Direction::IN;
		p5->setName("G");

		p3->addEventListener(navigator);

		Pin* p6 = new Pin(Pin::Type::EVENT);
		p6->direction = Pin::Direction::OUT;
		p6->setName("G");

		addPin(Pin::Direction::IN, p1);
		addPin(Pin::Direction::IN, p2);
		addPin(Pin::Direction::OUT, p3);
		addPin(Pin::Direction::OUT, p4);
		addPin(Pin::Direction::IN, p5);
		addPin(Pin::Direction::OUT, p6);

		p5->listeners.push_back(this);
	}

	void TrackIn::eventReceived(Event* e)
	{
		// recording case, send event to current region of track and to event out pin
		// this is necessary to update the midi regions in the timeline and to pass the event to the next module

		// Nur extern via Pin gelieferte Events aufnehmen. Track::notifyListeners
		// (Region-Playback, sendAllNotesOff) liefert Events ohne Source-Pin —
		// die duerfen nicht in die Recording-Region zurueckgeschrieben werden,
		// sonst dupliziert sich bei Overdub im Loop die Region bei jedem Pass.
		if (track != nullptr && track->isRecording() && e->getSource() != nullptr) {
			if (e->getMessage().getChannel() != track->getMidiChannel()) {
				delete e;
				return;
			}
			MidiRegion* r = dynamic_cast<MidiRegion*>(track->getRecordingRegion());
			if (r != nullptr) {
				long samplePosition = track->getNavigator()->getSamplePosition();
				MidiMessage* m = new MidiMessage(e->getMessage(), samplePosition - r->getSampleOffset());
				// add message to region
				r->addMessage(m, Time::getMillisecondCounterHiRes() / 1000, samplePosition);
			}
			else {
				// Logger::writeToLog("No region found at " + String(track->getSamplePosition()));
			}

		}

		if (track != nullptr && e->getMessage().getChannel() != track->getMidiChannel()) {
			delete e;
			return;
		}

		// Forward through the OUT pin so it lights up in the visual debugger
		// and so multiple downstream connections all receive the event (the
		// previous code only forwarded to the first connection).
		if (getPins().at(5)->getConnections().size() > 0) {
			getPins().at(5)->sendEvent(e);
		}
		else {
			delete e;
		}
		
	}

	void TrackIn::paint(juce::Graphics& g) {
		Module::paint(g);
		if (trackType == Track::Type::AUDIO) {
			g.drawImageAt(ImageCache::getFromMemory(BinaryData::micro_png, BinaryData::micro_pngSize), 25, 40);
		}
		else if (trackType == Track::Type::MIDI) {
			g.drawImageAt(ImageCache::getFromMemory(BinaryData::midiin_png, BinaryData::midiin_pngSize), 25, 40);
		}
	}


	void TrackIn::process() {

		if (trackType == Track::Type::AUDIO) {

			float* outL = nullptr;
			float* outR = nullptr;

			const float* inL = nullptr;
			const float* inR = nullptr;

			if (getPins().at(0)->getConnections().size() >= 1) {
				inL = getPins().at(0)->getConnections().at(0)->getAudioBuffer()->getReadPointer(0);
			}
			if (getPins().at(1)->getConnections().size() >= 1) {
				inR = getPins().at(1)->getConnections().at(0)->getAudioBuffer()->getReadPointer(0);
			}

			outL = getPins().at(2)->getAudioBuffer()->getWritePointer(0);
			outR = getPins().at(3)->getAudioBuffer()->getWritePointer(0);

			if (track != nullptr && (navigator->isPlaying() || isBouncing()
                || (navigator->isRecording() && !track->isRecording()))) {

				wasRecording = false;
				double pan = track->getPan();

				float gainLeft = cos((M_PI * (pan + 1) / 4));
				float gainRight = sin((M_PI * (pan + 1) / 4));

				// Mute / Solo: ohne diesen Faktor wirkten beide Toggles bisher
				// nirgends im Audio-Pfad — Track::isMute/isSolo wurden nur in
				// den Properties gespeichert. anyTrackSoloed() ist O(n_tracks),
				// aber nicht im inneren Sample-Loop.
				const bool anySolo = Mixer::getInstance()->anyTrackSoloed();
				const float muteSoloGain =
					(track->isMute() || (anySolo && !track->isSolo())) ? 0.0f : 1.0f;

				for (int j = track->getSamplePosition(); j < buffersize + track->getSamplePosition(); j++) {
					int bufferIndex = j - track->getSamplePosition(); // 0, 1, 2, ...

					float sampleL = track->getSample(0, j) * track->getVolume() * gainLeft  * muteSoloGain;
					float sampleR = track->getSample(1, j) * track->getVolume() * gainRight * muteSoloGain;

					pins.at(2)->getAudioBuffer()->setSample(0, bufferIndex, sampleL);
					pins.at(3)->getAudioBuffer()->setSample(0, bufferIndex, sampleR);
				}
				// track->setSamplePosition(track->getSamplePosition() + buffersize);
				// numSamples += buffersize;

				track->updateMagnitude(track->getSamplePosition(), buffersize, gainLeft, gainRight);
		
			}
			else if (track != nullptr && navigator->isRecording()) {

				// Reset the in-region buffer index at the start of each recording.
				// Otherwise the counter keeps climbing across stop/start cycles
				// and the next recording writes past the new region's buffer end
				// (or, combined with the old getRegionAt bug in Track::setSample,
				// into the previous region).
				if (!wasRecording) {
					currentRecordingSample = 0;
					numRecordedSamples = 0;
					wasRecording = true;
				}

				for (int i = 0; i < buffersize; i++) {

					if (inL != nullptr && track != nullptr) {
						track->setSample(0, currentRecordingSample, inL[i] * track->getVolume());
						outL[i] = inL[i] * track->getVolume();
					}
					else {

						// track->getRecordingBuffer()->setSample(0, currentRecordingSample, 0);
						outL[i] = 0;
					}

					if (inR != nullptr && track != nullptr) {
						track->setSample(1, currentRecordingSample, inR[i] * track->getVolume());
						outR[i] = inR[i] * track->getVolume();
					}
					else {
						// track->getRecordingBuffer()->setSample(1, currentRecordingSample, 0);
						outR[i] = 0;
					}

					currentRecordingSample = (currentRecordingSample + 1);
					numRecordedSamples++;

				}
			}

			else {
				wasRecording = false;
				for (int i = 0; i < buffersize; i++) {
					if (navigator->isRecording()) {					
						if (inL != nullptr && outL != nullptr) {
							outL[i] = inL[i];
						}
						if (inR != nullptr && outR != nullptr) {
							outR[i] = inR[i];
						}
					}
					else {
						if (outL != nullptr) {
							outL[i] = 0;
						}
						if (outR != nullptr) {
							outR[i] = 0;
						}
					}
				}
			}
		}
		else {
			if (track != nullptr && !track->isRecording()) {
				
			}
		}

	}

	void TrackIn::bounce()
	{
		if (trackType != Track::Type::AUDIO)
			return;

		// Defensive: BounceThread iteriert alle Top-Level-Module und ruft bounce()
		// auf jedem TrackIn. Wenn der User ueber die Toolbox einen losen "Track In"
		// platziert hat (PrefabFactory.cpp:309 default Track::Type::AUDIO, aber kein
		// setTrack-Aufruf) oder ein Project-Load die Track-Wiederverdrahtung
		// uebersprungen hat, ist track==null trotz trackType==AUDIO und der erste
		// track->getPan() unten knallt sauber durch das Bounce-Thread.
		if (track == nullptr) {
			juce::Logger::writeToLog(
				"TrackIn::bounce: skipping AUDIO TrackIn '" + getName()
				+ "' with no track assigned (orphan or load not rewired)");
			return;
		}

		// pins werden in configurePins() angelegt (6 Stueck). Wenn der TrackIn
		// noch nicht konfiguriert ist (z.B. waehrend einer Module-Replace-Aktion),
		// brechen wir ab statt mit std::out_of_range zu sterben.
		if (pins.size() < 4)
			return;

		float* outL = nullptr;
		float* outR = nullptr;

		const float* inL = nullptr;
		const float* inR = nullptr;

		if (getPins().at(0)->getConnections().size() >= 1) {
			inL = getPins().at(0)->getConnections().at(0)->getAudioBuffer()->getReadPointer(0);
		}
		if (getPins().at(1)->getConnections().size() >= 1) {
			inR = getPins().at(1)->getConnections().at(0)->getAudioBuffer()->getReadPointer(0);
		}

		outL = getPins().at(2)->getAudioBuffer()->getWritePointer(0);
		outR = getPins().at(3)->getAudioBuffer()->getWritePointer(0);

		double pan = track->getPan();

		float gainLeft = cos((M_PI * (pan + 1) / 4));
		float gainRight = sin((M_PI * (pan + 1) / 4));

		// Mute/Solo auch beim Bounce respektieren — sonst werden
		// stummgeschaltete Spuren ueberraschend mit ins Render geschrieben.
		const bool anySolo = Mixer::getInstance()->anyTrackSoloed();
		const float muteSoloGain =
			(track->isMute() || (anySolo && !track->isSolo())) ? 0.0f : 1.0f;

		for (int j = track->getSamplePosition(); j < buffersize + track->getSamplePosition(); j++) {
			int bufferIndex = j - track->getSamplePosition(); // 0, 1, 2, ...

			float sampleL = track->getSample(0, j) * track->getVolume() * gainLeft  * muteSoloGain;
			float sampleR = track->getSample(1, j) * track->getVolume() * gainRight * muteSoloGain;

			pins.at(2)->getAudioBuffer()->setSample(0, bufferIndex, sampleL);
			pins.at(3)->getAudioBuffer()->setSample(0, bufferIndex, sampleR);
		}

		track->updateMagnitude(track->getSamplePosition(), buffersize, gainLeft, gainRight);

		// Diagnose: nach dem Schreiben einmal pro Bounce-Block die Pin-Magnitude
		// loggen. Wenn diese > 0 ist und im BounceThread trotzdem nichts
		// ankommt, liegt das Problem hinter dem Pin (Connection/AudioOut-
		// Routing). Wenn schon hier 0 rauskommt, fehlen entweder Track-Daten
		// oder track->getVolume()/mute/solo nullifiziert das Signal.
		{
			float mag = pins.at(2)->getAudioBuffer()->getMagnitude(0, 0, buffersize);
			if (mag > 0.0001f || track->getSamplePosition() < buffersize * 4)
				DBG("TrackIn::bounce '" << getName() << "' track='" << track->getName()
					<< "' pos=" << track->getSamplePosition()
					<< " vol=" << track->getVolume()
					<< " pinMag=" << mag);
		}
	}

	void TrackIn::setChannelIndex(int index) {
		this->channelIndex = index;
	}

	void TrackIn::setSamplePosition(int samples)
	{
		offset = samples;
	}

	Track* TrackIn::getTrack()
	{
		return track;
	}

	void TrackIn::setTrack(Track* t)
	{
		this->track = t;
		trackType = t->getType();	
		t->AddEventListener(this);
		
	}

	int TrackIn::getChannelIndex() {
		return channelIndex;
	}

	juce::Array<PropertyComponent*>& TrackIn::getProperties() {

		Array<var> values = Array<var>();

		StringArray s = navigator->getTrackList();

		for (int i = 0; i < s.size(); i++) {
			values.add(s.getReference(i));
		}

		properties = juce::Array<PropertyComponent*>();
		volumeProp = new SliderPropertyComponent(*volumeValue, "Volume", 0, 1, 0.01, 1.0, true);
		trackProp = new ChoicePropertyComponent(*trackValue, "Track", navigator->getTrackList(), values);
		properties.add(volumeProp);
		properties.add(trackProp);
		return properties;
	}


