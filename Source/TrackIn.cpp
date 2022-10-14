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
TrackIn::TrackIn(TrackNavigator* navigator, double sampleRate, int buffersize)
{
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
	p1->direction = Pin::Direction::IN;
	p1->setName("L");

	Pin* p2 = new Pin(Pin::Type::AUDIO);
	p2->direction = Pin::Direction::IN;
	p2->setName("R");

	addPin(Pin::Direction::OUT, p1);
	addPin(Pin::Direction::OUT, p2);
}

void TrackIn::paint(juce::Graphics& g) {
	Module::paint(g);
	g.drawImageAt(ImageCache::getFromMemory(BinaryData::micro_png, BinaryData::micro_pngSize), 25, 40);
}

void TrackIn::process() {

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
	else {
		pins.at(0)->getAudioBuffer()->clear();
		pins.at(1)->getAudioBuffer()->clear();
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


