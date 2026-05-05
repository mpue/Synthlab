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

		if (track != nullptr && track->isRecording()) {
			if (e->getMessage().getChannel() != track->getMidiChannel()) {
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
			return;
		}

		// check if pin is connected
		if (getPins().at(5)->getConnections().size() > 0) {
			getPins().at(5)->getConnections().at(0)->sendEvent(e);
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
				for (int j = track->getSamplePosition(); j < buffersize + track->getSamplePosition(); j++) {
					int bufferIndex = j - track->getSamplePosition(); // 0, 1, 2, ...

					float sampleL = track->getSample(0, j) * track->getVolume() * gainLeft;
					float sampleR = track->getSample(1, j) * track->getVolume() * gainRight;

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

			double pan = track->getPan();

			float gainLeft = cos((M_PI * (pan + 1) / 4));
			float gainRight = sin((M_PI * (pan + 1) / 4));
			for (int j = track->getSamplePosition(); j < buffersize + track->getSamplePosition(); j++) {
				int bufferIndex = j - track->getSamplePosition(); // 0, 1, 2, ...

				float sampleL = track->getSample(0, j) * track->getVolume() * gainLeft;
				float sampleR = track->getSample(1, j) * track->getVolume() * gainRight;

				pins.at(2)->getAudioBuffer()->setSample(0, bufferIndex, sampleL);
				pins.at(3)->getAudioBuffer()->setSample(0, bufferIndex, sampleR);
			}

			track->updateMagnitude(track->getSamplePosition(), buffersize, gainLeft, gainRight);
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


