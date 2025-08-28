/*
  ==============================================================================

	Track.cpp
	Created: 16 Dec 2016 9:40:38am
	Author:  pueskma

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "Track.h"
#include "../Session.h"
#include "MasterChannelPanel.h"
#include <iostream>

//==============================================================================
Track::Track(Type type, double sampleRate, MultiComponentDragger* dragger)
{
	this->constrainer = new ResizeConstrainer(this->zoom / 4);
	resizer = new ResizableEdgeComponent(this, constrainer, ResizableEdgeComponent::bottomEdge);
	resizer->setSize(getWidth(), 5);
	constrainer->setMaxWidth(1000);
	resizer->setTopLeftPosition(0, 70);
	manager = Session::getInstance()->getAudioFormatManager();
	this->type = type;
	this->sampleRate = sampleRate;
	this->maxLength = Project::getInstance()->getTrackLength();
	this->volume = 1;
	this->pan = 0;
	this->audioBuffer = new AudioSampleBuffer(2, maxLength * sampleRate);
	this->dragger = dragger;
	this->midiChannel = 1;
	this->bufferSize = Project::getInstance()->getBufferSize();
	addAndMakeVisible(resizer);	
	setHeight(Project::DEFAULT_TRACK_HEIGHT);
	setMouseClickGrabsKeyboardFocus(true);
	resized();
	createProperties();
	setName("empty Track");
}

Track::~Track()
{
	for (std::vector<Region*>::iterator it = regions.begin(); it != regions.end(); ++it) {
		delete* it;
	}
	delete this->resizer;
	delete this->constrainer;
	delete this->audioBuffer;
	delete nameListener;
	delete nameValue;
	delete recordingListener;
	delete recordingValue;
	delete typeListener;
	delete typeValue;
	delete channelListener;
	delete channelValue;
}

void Track::setName(juce::String name) {
	Component::setName(name);
	this->name = name;
	sendChangeMessage();
}

void Track::clearSelection() {
	for (std::vector<Region*>::iterator it = regions.begin(); it != regions.end(); ++it) {
		(*it)->setSelected(false);
	}
}

MidiBuffer* Track::getMidiBuffer()
{
	return this->midiBuffer;
}

void Track::toggleLoopSelection() {
	for (std::vector<Region*>::iterator it = regions.begin(); it != regions.end(); ++it) {
		if ((*it)->isSelected())
			(*it)->setLoop(!(*it)->isLoop());
	}
}

void Track::removeSelectedRegions(bool clear) {

	for (std::vector<Region*>::iterator it = regions.begin(); it != regions.end();) {
		if (dragger->isSelected((*it))) {
			(*it)->removeAllChangeListeners();
			if (clear)
				audioBuffer->clear((*it)->getSampleOffset(), (*it)->getNumSamples());
			delete* it;
			it = regions.erase(it);
		}
		else {
			++it;
		}
	}

}

void Track::copySelectedRegions() {

	for (std::vector<Region*>::iterator it = regions.begin(); it != regions.end(); ++it) {
		if (dragger->isSelected((*it)))
			Session::getInstance()->copyRegion((*it));
	}

}

void Track::duplicateSelectedRegions() {

	vector<Region*> selected;

	for (std::vector<Region*>::iterator it = regions.begin(); it != regions.end(); ++it) {
		if (dragger->isSelected((*it)))
			selected.push_back((*it));
	}

	for (int i = 0; i < selected.size(); i++) {
		duplicateRegion(selected.at(i));
	}

	selected.clear();

}

void Track::duplicateRegion(Region* region) {

	if (AudioRegion* _region = dynamic_cast<AudioRegion*>(region)) {

		AudioRegion* duplicate = new AudioRegion(_region, *manager, sampleRate);
		duplicate->setDragger(dragger);
		Rectangle<int>* bounds = new Rectangle<int>(0, 0, _region->getThumbnail()->getTotalLength() * 20, getHeight());
		duplicate->setBounds(region->getWidth() + region->getX(), 0, region->getWidth(), region->getHeight());
		duplicate->setThumbnailBounds(bounds);
		duplicate->setLoopCount(0);

		this->regions.push_back(duplicate);
		this->currentRegion = duplicate;

		this->currentRegion->toFront(true);
		addAndMakeVisible(duplicate);

		clearSelection();
		dragger->setSelected(duplicate, true);
		//duplicate->setSelected(true);
		duplicate->setSampleOffset(region->getSampleOffset() + region->getNumSamples(), false, false);
		duplicate->setOffset(region->getOffset() + region->getWidth());
		duplicate->setClipRefId(_region->getClipRefid());
		duplicate->addChangeListener(this);

		if (zoom > 0)
			duplicate->setZoom(zoom);

		audioBuffer->copyFrom(0, duplicate->getSampleOffset(), *_region->getBuffer(), 0, 0, _region->getNumSamples());
		audioBuffer->copyFrom(1, duplicate->getSampleOffset(), *_region->getBuffer(), 1, 0, _region->getNumSamples());

		repaint();
	}
	else if (MidiRegion* mregion = dynamic_cast<MidiRegion*>(region)) {
		long newOffset = mregion->getSampleOffset() + mregion->getNumSamples();
		MidiRegion* clone = mregion->clone(newOffset);
		clone->setDragger(dragger);
		clone->setBounds(region->getX() + region->getWidth(), 0, region->getWidth(), getHeight());
		clone->addChangeListener(this);

		this->regions.push_back(clone);
		this->currentRegion = clone;
		addAndMakeVisible(clone);
		clearSelection();
		dragger->setSelected(clone, true);
		repaint();
	}
}

Region* Track::addRegion(String refId, File file, double sampleRate, long sampleOffset) {
	AudioRegion* region = new AudioRegion(file, refId, *manager, sampleRate);
	region->setDragger(dragger);
	Rectangle<int>* bounds = new Rectangle<int>(0, 0, region->getWidth() * zoom, getHeight());
	region->setBounds(markerPosition, 0, region->getWidth(), getHeight());
	region->setThumbnailBounds(bounds);
	region->setLoopCount(0);

	if (regions.size() == 0) {
		setName(region->getName());
	}

	this->regions.push_back(region);
	this->currentRegion = region;
	region->addChangeListener(this);

	this->currentRegion->toFront(true);
	addAndMakeVisible(region);

	clearSelection();
	region->setSelected(true);
	region->setDirty(true);

	// long sampleNum = (this->maxLength / (this->maxLength * zoom)) * markerPosition * sampleRate;
	region->setSampleOffset(sampleOffset, false, false);
	region->setOffset(markerPosition);

	if (zoom > 0)
		region->setZoom(zoom);

	audioBuffer->copyFrom(0, region->getSampleOffset(), *region->getBuffer(), 0, 0, region->getBuffer()->getNumSamples());
	audioBuffer->copyFrom(1, region->getSampleOffset(), *region->getBuffer(), 1, 0, region->getBuffer()->getNumSamples());

	repaint();

	long tracklen = Project::getInstance()->getTrackLength();
	float x = (sampleOffset / (tracklen / (tracklen * zoom))) / sampleRate;

	region->setTopLeftPosition(Point<int>(x, region->getBounds().getY()));
	return region;
}

void Track::addRegion(String refId, File file, double sampleRate) {

	AudioRegion* region = new AudioRegion(file, refId, *manager, sampleRate);
	region->setDragger(dragger);
	Rectangle<int>* bounds = new Rectangle<int>(0, 0, region->getWidth() * zoom, getHeight());
	region->setBounds(markerPosition, 0, region->getWidth(), getHeight());
	region->setThumbnailBounds(bounds);
	region->setLoopCount(0);

	if (regions.size() == 0) {
		setName(region->getName());
	}

	this->regions.push_back(region);
	this->currentRegion = region;
	region->addChangeListener(this);

	this->currentRegion->toFront(true);
	addAndMakeVisible(region);

	clearSelection();
	region->setSelected(true);
	region->setDirty(true);

	long sampleNum = (this->maxLength / (this->maxLength * zoom)) * markerPosition * sampleRate;
	region->setSampleOffset(sampleNum, false, false);
	region->setOffset(markerPosition);

	if (zoom > 0)
		region->setZoom(zoom);

	audioBuffer->copyFrom(0, region->getSampleOffset(), *region->getBuffer(), 0, 0, region->getBuffer()->getNumSamples());
	audioBuffer->copyFrom(1, region->getSampleOffset(), *region->getBuffer(), 1, 0, region->getBuffer()->getNumSamples());

	repaint();
}

void Track::addRegion(juce::String refId, juce::File file, double sampleRate, long samplePosition, long regionLength) {
	AudioRegion* region = new AudioRegion(file, refId, *manager, sampleRate);
	region->setDragger(dragger);
	Rectangle<int>* bounds = new Rectangle<int>(0, 0, region->getWidth() * zoom, getHeight());
	region->setBounds(markerPosition, 0, region->getWidth(), getHeight());
	region->setThumbnailBounds(bounds);
	region->setLoopCount(0);

	if (regions.size() == 0) {
		setName(region->getName());
	}

	this->regions.push_back(region);
	this->currentRegion = region;
	region->addChangeListener(this);

	this->currentRegion->toFront(true);
	addAndMakeVisible(region);

	clearSelection();
	region->setSelected(true);

	region->setSampleOffset(samplePosition, false, false);
	region->setOffset(samplePosition / sampleRate);

	if (zoom > 0)
		region->setZoom(zoom);

	audioBuffer->copyFrom(0, region->getSampleOffset(), *region->getBuffer(), 0, 0, region->getBuffer()->getNumSamples());
	audioBuffer->copyFrom(1, region->getSampleOffset(), *region->getBuffer(), 1, 0, region->getBuffer()->getNumSamples());

	repaint();
}

MidiRegion* Track::addMidiRegion(double sampleRate, long samplePosition, long regionLength) {

	Logger::getCurrentLogger()->writeToLog("Adding MidiRegion at " + String(samplePosition) + " with length " + String(regionLength));

	MidiRegion* region = new MidiRegion(samplePosition, regionLength, sampleRate, dragger);
	
	region->setLoopCount(0);

	if (regions.size() == 0) {
		setName(region->getName());
	}

	this->regions.push_back(region);
	this->currentRegion = region;
	region->addChangeListener(this);

	this->currentRegion->toFront(true);
	addAndMakeVisible(region);

	clearSelection();
	region->setSelected(true);

	/*
	if (zoom > 0)
		region->setZoom(zoom);
	*/

	region->updateThumb();

	repaint();

	return region;
}

void Track::addMidiRegion(juce::MidiBuffer* source, double sampleRate, long startPosition)
{
	MidiRegion* region = new MidiRegion(source, (long)sampleRate);
	region->setDragger(dragger);
	Rectangle<int>* bounds = new Rectangle<int>(0, 0, region->getThumbnail()->getTotalLength() * 20, getHeight());
	region->setBounds(startPosition, 0, region->getWidth(), getHeight());
	region->setThumbnailBounds(bounds);
	region->setLoopCount(0);

	if (regions.size() == 0) {
		setName(region->getName());
	}

	this->regions.push_back(region);
	this->currentRegion = region;
	region->addChangeListener(this);

	this->currentRegion->toFront(true);
	addAndMakeVisible(region);

	clearSelection();
	region->setSelected(true);

	region->setSampleOffset(0, false, false);
	region->setOffset(0);
	region->repaint();
	if (zoom > 0)
		region->setZoom(zoom);

	repaint();
}

void Track::addRegion(AudioSampleBuffer* source, double sampleRate, long samplePosition, long regionLength) {

	AudioRegion* region = new AudioRegion(source, *manager, samplePosition, regionLength, sampleRate);
	region->setDragger(dragger);
	Rectangle<int>* bounds = new Rectangle<int>(0, 0, (region->getBuffer()->getNumSamples() / sampleRate) * this->zoom, getHeight());
	region->setBounds(markerPosition, 0, region->getBuffer()->getNumSamples() / sampleRate, getHeight());
	region->setThumbnailBounds(bounds);
	region->setLoopCount(0);
	region->updateThumb();

	if (regions.size() == 0) {
		setName(region->getName());
	}

	this->regions.push_back(region);
	this->currentRegion = region;
	region->addChangeListener(this);

	this->currentRegion->toFront(true);
	addAndMakeVisible(region);

	clearSelection();
	region->setSelected(true);

	region->setSampleOffset(samplePosition, false, false);
	region->setOffset(samplePosition / sampleRate);

	if (zoom > 0)
		region->setZoom(zoom);

	audioBuffer->copyFrom(0, region->getSampleOffset(), *region->getBuffer(), 0, 0, region->getBuffer()->getNumSamples());
	audioBuffer->copyFrom(1, region->getSampleOffset(), *region->getBuffer(), 1, 0, region->getBuffer()->getNumSamples());

	repaint();
}

void Track::splitRegion() {

	long sampleNum = getSamplePosition();
	
	Region* baseRegion = getRegionAt(sampleNum);
	if (!baseRegion || !baseRegion->isSelected())
		return;

	if (AudioRegion* region = dynamic_cast<AudioRegion*>(baseRegion)) {

		if (region == NULL) {
			return;
		}

		if (region->isSelected()) {

			long numLeftSamples = sampleNum - region->getSampleOffset();
			long numRightSamples = region->getNumSamples() - numLeftSamples;

			AudioRegion* leftRegion = new AudioRegion(region, *manager, sampleRate, 0, numLeftSamples);
			AudioRegion* rightRegion = new AudioRegion(region, *manager, sampleRate, numLeftSamples, numRightSamples);

			leftRegion->setDragger(dragger);
			rightRegion->setDragger(dragger);

			Rectangle<int>* leftBounds = new Rectangle<int>(0, 0, leftRegion->getThumbnail()->getTotalLength() * 20, getHeight());
			Rectangle<int>* rightBounds = new Rectangle<int>(0, 0, rightRegion->getThumbnail()->getTotalLength() * 20, getHeight());

			leftRegion->setBounds(region->getX(), 0, leftRegion->getWidth(), getHeight());
			leftRegion->setThumbnailBounds(leftBounds);

			rightRegion->setBounds(markerPosition, 0, rightRegion->getWidth(), getHeight());
			rightRegion->setThumbnailBounds(rightBounds);

			this->regions.push_back(leftRegion);
			leftRegion->addChangeListener(this);
			this->regions.push_back(rightRegion);
			rightRegion->addChangeListener(this);

			addAndMakeVisible(leftRegion);
			addAndMakeVisible(rightRegion);

			if (dragger->isSelected(region))
				dragger->setSelected(region, false);

			leftRegion->setSampleOffset(region->getSampleOffset(), false, false);
			leftRegion->setOffset(region->getOffset());

			rightRegion->setSampleOffset(sampleNum, false, false);
			rightRegion->setOffset((double)sampleNum / sampleRate);

			if (zoom > 0) {
				leftRegion->setZoom(zoom);
				rightRegion->setZoom(zoom);
			}

			clearSelection();

			region->setSelected(true);
			dragger->setSelected(region, true);

			removeSelectedRegions(false);

			leftRegion->setSelected(true);
			rightRegion->setSelected(true);
		}

	}
	
	if (MidiRegion* region = dynamic_cast<MidiRegion*>(baseRegion)) {
		auto [leftRegion, rightRegion] = region->splitAt(sampleNum, dragger);
		if (!leftRegion || !rightRegion)
			return;

		const int leftX = region->getX(); // Original X bleibt
		const int rightX = (int)((sampleNum / sampleRate) * zoom);  // ✅ korrekt berechnetes X

		const int leftW = (int)((leftRegion->getNumSamples() / sampleRate) * zoom);
		const int rightW = (int)((rightRegion->getNumSamples() / sampleRate) * zoom);

		leftRegion->setBounds(leftX, 0, leftW, getHeight());
		rightRegion->setBounds(rightX, 0, rightW, getHeight());

		leftRegion->setDragger(dragger);
		rightRegion->setDragger(dragger);

		if (dragger->isSelected(region))
			dragger->setSelected(region, false);

		// Entferne alte Region
		auto it = std::find(regions.begin(), regions.end(), region);
		if (it != regions.end()) {
			region->removeAllChangeListeners();
			removeChildComponent(region);
			delete region;
			regions.erase(it);
		}

		// Sichtbar machen und registrieren
		leftRegion->addChangeListener(this);
		rightRegion->addChangeListener(this);
		addAndMakeVisible(leftRegion);
		addAndMakeVisible(rightRegion);
		regions.push_back(leftRegion);
		regions.push_back(rightRegion);

		clearSelection();
		dragger->setSelected(leftRegion, true);
		dragger->setSelected(rightRegion, true);
		repaint();
	}


}

void Track::setSamplePosition(int position)
{
	if ((type == MIDI || type == INSTRUMENT) && position != samplePosition) {
			
		MidiRegion* r = dynamic_cast<MidiRegion*>(getRegionAt(position));
		if (r != nullptr) {
			vector<MidiMessage*> messages = r->getMessages(position - r->getSampleOffset(), 64);
			
			for (auto* m : messages) {
				if (m != nullptr) {
					// Logger::getCurrentLogger()->writeToLog("Midi message at " + String(position) + " : " + String(m->getDescription()));
					Event* e = new Event();
					e->setMessage(*m);
					e->setType(Event::Type::GATE);
					e->setNote(m->getNoteNumber());
					if (m->getVelocity() > 127) {
						m->setVelocity(127);
					}
					e->setValue(m->isNoteOn() ? m->getVelocity() : 0);
					// send event to all connected pins
					notifyListeners(e);					
				}
			}
		}
	}
	this->samplePosition = position;
}

int Track::getSamplePosition()
{
	return samplePosition;
}

void Track::AddEventListener(EventListener* listener)
{
	listeners.push_back(listener);
}

void Track::notifyListeners(Event* e)
{
	for (int i = 0; i < listeners.size(); i++) {
		listeners.at(i)->eventReceived(new Event(e));
	}
	delete e;
}

void Track::setZoom(float zoom)
{
	this->zoom = zoom;

	Logger::getCurrentLogger()->writeToLog(String(zoom));

	for (std::vector<Region*>::iterator it = regions.begin(); it != regions.end(); ++it) {
		(*it)->setZoom(zoom);
	}
}

String Track::getName()
{
	return name;
}

float Track::getGain() {
	return gain;
}

void Track::setGain(float gain)
{
	this->gain = gain;
}

void Track::setVolume(float volume)
{
	this->volume = volume;
}

float Track::getVolume()
{
	return volume;
}

void Track::setPan(float pan)
{
	this->pan = pan;
}

float Track::getPan()
{
	return pan;
}

Region* Track::getRegionAt(long sample) {

	for (std::vector<Region*>::iterator it = regions.begin(); it != regions.end(); ++it) {
		Region* r = *it;
		long len = r->getNumSamples();
		long offset = r->getSampleOffset();

		if (sample >= offset && sample < (offset + len)) {
			return r;
		}
	}

	return nullptr;
}

Region* Track::getCurrentRegion()
{
	Region* current = nullptr;
	current = getRegionAt(samplePosition);
	return current;
}

const float Track::getSample(int channel, long sample) {
	if (mute) {
		return 0;
	}
	currentRegion = getRegionAt(sample);

	if (currentRegion != nullptr) {
		if (sample - currentRegion->getSampleOffset() < getBuffer()->getNumSamples() && sample - currentRegion->getSampleOffset() > 0) {
			return getBuffer()->getSample(channel, sample - currentRegion->getSampleOffset());
		}
	}
	return 0;
}

juce::Array<juce::PropertyComponent*>& Track::getProperties()
{
	properties = Array<PropertyComponent*>();

	nameProp = new TextPropertyComponent(*nameValue, "Name", 16, false, true);
	properties.add(nameProp);

	recordingProp = new juce::BooleanPropertyComponent(*recordingValue, "recording", "Recording");
	properties.add(recordingProp);

	typeProp = new juce::ChoicePropertyComponent(*typeValue, "Type", getTypeNames(), getTypeValues());
	properties.add(typeProp);

	channelProp = new SliderPropertyComponent(*channelValue, "channel", 1.0f, 16.0f, 1, 1, true);
	properties.add(channelProp);

	typeValue->setValue(getTypeValues().indexOf(type));

	return properties;
}

void Track::createProperties()
{
	nameValue = new Value();
	nameListener = new NameListener(*nameValue, this);

	recordingValue = new Value();
	recordingListener = new RecordingListener(*recordingValue, this);

	typeValue = new Value();
	typeListener = new TypeListener(*typeValue, this);

	channelValue = new Value();
	channelValue->setValue(midiChannel);
	channelListener = new MIdiChannelListener(*channelValue, this);
}

void Track::setSample(int channel, int sampleNum, float sample)
{
	currentRegion = getRegionAt(sample);

	if (currentRegion != nullptr) {
		AudioSampleBuffer* buf = getBuffer();
		if (buf != nullptr) {
			if (sampleNum >= buf->getNumSamples()) {
				AudioRegion* ar = dynamic_cast<AudioRegion*>(currentRegion);
				// TODO : from card
				ar->resizeAudio(256);
			}
			buf = getBuffer(); // reassign, because of resizing
			buf->setSample(channel, sampleNum, sample);
			currentRegion->setDirty(true);
		}
	}

}

void Track::setMagnitude(int channel, double magnitude) {
	if (channel == 0) {
		this->magnitudeLeft = magnitude;
	}
	else {
		this->magnitudeRight = magnitude;
	}
}

double Track::getMagnitude(int channel) {
	if (channel == 0) {
		return this->magnitudeLeft;
	}
	else {
		return this->magnitudeRight;
	}
}

void Track::updateMagnitude(int sample, int buffersize, float gainLeft, float gainRight) {
	this->magnitudeLeft = audioBuffer->getMagnitude(0, sample, buffersize) * gainLeft;
	this->magnitudeRight = audioBuffer->getMagnitude(1, sample, buffersize) * gainRight;
}

const float* Track::getReadBuffer(int channel) {

	if (type == AUDIO) {
		AudioRegion* region = static_cast<AudioRegion*>(currentRegion);
		return region->getBuffer()->getReadPointer(channel);
	}

	return NULL;
}

int Track::getNumSamples() {
	return numSamples;
}

void Track::setSelected(bool selected) {
	this->selected = selected;
	sendChangeMessage();
	repaint();
}

bool Track::isSelected() {
	return selected;
}

double Track::getMaxLength() {
	return maxLength;
}

void Track::paint(Graphics& g) {

	if (selected) {
		g.setColour(Colours::darkgrey);
	}
	else {
		g.setColour(Colours::darkgrey.darker());
	}

	g.fillAll();
	g.setColour(Colours::grey);
	g.drawLine(0, getHeight(), getWidth(), getHeight());
}

void Track::resized() {
	for (std::vector<Region*>::iterator it = regions.begin(); it != regions.end(); ++it) {
		(*it)->setSize((*it)->getWidth(), getHeight());
		(*it)->setZoom(zoom);
	}
	resizer->setSize(getWidth(), 5);
	resizer->setTopLeftPosition(0, getHeight() - 5);
	constrainer->setMaxWidth(getWidth());
	sendChangeMessage();
}

void Track::setOffset(int offset)
{
	this->offset = offset;
}

int Track::getOffset()
{
	return this->offset;
}


void Track::setHeight(int height)
{
	Component::setSize(getWidth(), height);
	this->height = height;
}

AudioSampleBuffer* Track::getBuffer()
{
	if (type == AUDIO) {
		AudioRegion* region = static_cast<AudioRegion*>(currentRegion);
		if (region != nullptr) {
			return region->getBuffer();
		}
	}

	return nullptr;
}

AudioSampleBuffer* Track::getRecordingBuffer()
{
	return this->getBuffer();
}

void Track::setCurrentMarkerPosition(int position) {
	this->markerPosition = position;
	// Logger::getCurrentLogger()->writeToLog("Track " + String(midiChannel) + " marker : "+ String(position));
}

int Track::getMidiChannel() {
	return midiChannel;
}

void Track::setMidiChannel(int channel, bool notify) {
	this->midiChannel = channel;
	if (notify) {
		sendChangeMessage();
	}
}

void Track::changeListenerCallback(ChangeBroadcaster* source) {

	if (AudioRegion* r = dynamic_cast<AudioRegion*>(source)) {
		// cleanup old range first
		long oldoffset = r->getOldOffset();
		if (oldoffset < 0) {
			oldoffset = 0;
		}
		long sampleOffset = r->getSampleOffset();
		if (sampleOffset < 0) {
			sampleOffset = 0;
		}
		audioBuffer->clear(oldoffset, r->getBuffer()->getNumSamples());
		// copy data from region to tracks audio buffer
		long numSamples = r->getBuffer()->getNumSamples();
		audioBuffer->copyFrom(0, sampleOffset, *r->getBuffer(), 0, 0, numSamples);
		audioBuffer->copyFrom(1, sampleOffset, *r->getBuffer(), 1, 0, numSamples);
	}
	if (MasterChannelPanel* panel = dynamic_cast<MasterChannelPanel*>(source)) {
		setVolume(panel->getVolume());
		setPan(panel->getPan());
		sendChangeMessage();
	}

}

vector<Region*>Track::getRegions() {
	return this->regions;
}

void Track::setTrackLength(long length) {
	this->maxLength = length;
}

void Track::setRecording(bool recording) {
	this->recording = recording;
}

bool Track::isRecording() {
	return recording;
}

void Track::setSolo(bool solo) {
	this->solo = solo;
}

bool Track::isSolo() {
	return solo;
}

void Track::setMute(bool mute) {
	this->mute = mute;
}

bool Track::isMute() {
	return mute;
}

void Track::setMono(bool mono) {
	this->mono = mono;
}

bool Track::isMono() {
	return mono;
}
