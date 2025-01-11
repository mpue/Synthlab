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

	nameLabel->setJustificationType(Justification::left);
	nameLabel->setTopLeftPosition(18, 2);
	setName("Track in");

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
	p1->direction = Pin::Direction::OUT;
	p1->setName("L");

	Pin* p2 = new Pin(Pin::Type::AUDIO);
	p2->direction = Pin::Direction::OUT;
	p2->setName("R");

	Pin* p3 = new Pin(Pin::Type::AUDIO);
	p3->direction = Pin::Direction::IN;
	p3->setName("L");

	Pin* p4 = new Pin(Pin::Type::AUDIO);
	p3->direction = Pin::Direction::IN;
	p3->setName("R");


	addPin(Pin::Direction::OUT, p1);
	addPin(Pin::Direction::OUT, p2);
	addPin(Pin::Direction::IN, p3);
	addPin(Pin::Direction::IN, p4);

}

void TrackIn::paint(juce::Graphics& g) {
	Module::paint(g);
	g.drawImageAt(ImageCache::getFromMemory(BinaryData::micro_png, BinaryData::micro_pngSize), 25, 40);
}

void TrackIn::process() {

	if (trackType == Track::Type::AUDIO) {

		float* outL = nullptr;
		float* outR = nullptr;

		const float* inL = nullptr;
		const float* inR = nullptr;

		if (getPins().at(2)->getConnections().size() >= 1) {
			inL = getPins().at(2)->getConnections().at(0)->getAudioBuffer()->getReadPointer(0);
		}
		if (getPins().at(3)->getConnections().size() >= 1) {
			inR = getPins().at(3)->getConnections().at(0)->getAudioBuffer()->getReadPointer(0);
		}

		outL = getPins().at(0)->getAudioBuffer()->getWritePointer(0);
		outR = getPins().at(1)->getAudioBuffer()->getWritePointer(0);

		if (track != nullptr && navigator->isPlaying()) {

			double pan = track->getPan();

			float gainLeft = cos((M_PI * (pan + 1) / 4));
			float gainRight = sin((M_PI * (pan + 1) / 4));

			for (int j = track->getSamplePosition(); j <  buffersize + track->getSamplePosition(); j++) {

				float sampleL = track->getSample(0, j) * track->getVolume() * gainLeft;
				float sampleR = track->getSample(1, j) * track->getVolume() * gainRight;

				pins.at(0)->getAudioBuffer()->setSample(0, j % buffersize, sampleL);
				pins.at(1)->getAudioBuffer()->setSample(0, j % buffersize, sampleR);
			}
			// track->setSamplePosition(track->getSamplePosition() + buffersize);
			// numSamples += buffersize;

			track->updateMagnitude(track->getSamplePosition(), buffersize, gainLeft, gainRight);
		
		}
		else if (track != nullptr && navigator->isRecording()) {

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
			for (int i = 0; i < buffersize; i++) {
				
				if (inL != nullptr && outL != nullptr) {
					outL[i] = inL[i];
				}
				if (inR != nullptr && outR != nullptr) {
					outR[i] = inR[i];
				}
			}
		}
	}
	else if (trackType == Track::Type::MIDI) {

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


