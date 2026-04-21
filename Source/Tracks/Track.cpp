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
#include "../MessageBus/FastBus.h"

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
	// setMouseClickGrabsKeyboardFocus(true);
	resized();
	createProperties();

	if (type != AUDIO) {
		setName("Audio Track");	
	}
	else {
		setName("MIDI Track");
	}

	auto& bus = FastBus::getInstance();

	bus.subscribe<float>("tempo", [this](const FastTopic<float>& topic) {
		bpm = topic.getValue();
		repaint();
	});
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

		// Berechne die tatsächliche Anzahl der Samples (berücksichtigt Resizing)
		int numSamplesToCopy = _region->getNumSamples();

		// Erstelle Duplikat mit NUR den tatsächlich verwendeten Samples
		// Verwende den Konstruktor, der startSample und numSamples akzeptiert
		AudioRegion* duplicate = new AudioRegion(_region, *manager, sampleRate, 0, numSamplesToCopy);
		duplicate->setDragger(dragger);

		// Berechne die neue X-Position: direkt nach der ursprünglichen Region
		int newX = region->getX() + region->getWidth();

		// Berechne Sample-Offset aus der X-Position (in Pixeln)
		long newSampleOffset = (long)((newX / zoom) * sampleRate);

		// WICHTIG: Setze Bounds NACHDEM der Konstruktor die initiale Größe gesetzt hat
		// und überschreibe die Breite mit der tatsächlichen Breite der Originalregion
		Rectangle<int>* bounds = new Rectangle<int>(0, 0, region->getWidth(), getHeight());
		duplicate->setBounds(newX, 0, region->getWidth(), region->getHeight());
		duplicate->setThumbnailBounds(bounds);
		duplicate->setLoopCount(0);

		this->regions.push_back(duplicate);
		this->currentRegion = duplicate;

		this->currentRegion->toFront(true);
		addAndMakeVisible(duplicate);

		clearSelection();
		dragger->setSelected(duplicate, true);

		// Setze den Sample-Offset und visuellen Offset
		duplicate->setSampleOffset(newSampleOffset, false, false);
		duplicate->setOffset(newX);
		duplicate->setClipRefId(_region->getClipRefid());
		duplicate->addChangeListener(this);

		// WICHTIG: setZoom NACH setSampleOffset aufrufen
		// Dadurch wird die Position korrekt neu berechnet
		if (zoom > 0)
			duplicate->setZoom(zoom);

		// Kopiere nur die tatsächlich verwendeten Samples in den Track-Buffer
		if (duplicate->getBuffer()->getNumChannels() == 1) {
			audioBuffer->copyFrom(0, newSampleOffset, *duplicate->getBuffer(), 0, 0, numSamplesToCopy);
			audioBuffer->copyFrom(1, newSampleOffset, *duplicate->getBuffer(), 0, 0, numSamplesToCopy);
		}
		else {
			audioBuffer->copyFrom(0, newSampleOffset, *duplicate->getBuffer(), 0, 0, numSamplesToCopy);
			audioBuffer->copyFrom(1, newSampleOffset, *duplicate->getBuffer(), 1, 0, numSamplesToCopy);
		}

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

	// convert to stereo if there's only one channel
	// TODO : What would be a better solution, keep them as mono and handle each case?
	if (region->getBuffer()->getNumChannels() == 1) {
		audioBuffer->copyFrom(0, region->getSampleOffset(), *region->getBuffer(), 0, 0, region->getBuffer()->getNumSamples());
		audioBuffer->copyFrom(1, region->getSampleOffset(), *region->getBuffer(), 0, 0, region->getBuffer()->getNumSamples());
	}
	else {
		audioBuffer->copyFrom(0, region->getSampleOffset(), *region->getBuffer(), 0, 0, region->getBuffer()->getNumSamples());
		audioBuffer->copyFrom(1, region->getSampleOffset(), *region->getBuffer(), 1, 0, region->getBuffer()->getNumSamples());
	}

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

AudioRegion* Track::addRegion(AudioSampleBuffer* source, double sampleRate, long samplePosition, long regionLength) {

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

	return region;
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
		return;
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

		// If the active region changed (ended, seeked, or looped back),
		// stop all currently playing notes to prevent stuck notes.
		if (r != lastActiveRegion && !activeNoteNumbers.empty()) {
			sendAllNotesOff();
		}
		lastActiveRegion = r;

		if (r != nullptr) {
			// Use exact bufferSize window – bufferSize*2 caused each message to
			// appear in two consecutive windows, firing Note-Ons twice but
			// Note-Offs only once, resulting in stuck notes.
			vector<MidiMessage*> messages = r->getMessages(position - r->getSampleOffset(),
				Project::getInstance()->getBufferSize());

			for (auto* m : messages) {
				if (m != nullptr) {
					Event* e = new Event();
					e->setMessage(*m);
					e->setType(Event::Type::GATE);
					e->setNote(m->getNoteNumber());
					if (m->getVelocity() > 127)
						m->setVelocity(127);
					e->setValue(m->isNoteOn() ? m->getVelocity() : 0);

					// Track which notes are currently on.
					// Use find+erase(iterator) so that one NoteOff removes exactly
					// one entry from the multiset - important when two NoteOns for
					// the same pitch overlap (duplicate notes in the piano roll).
					if (m->isNoteOn() && m->getVelocity() > 0) {
						activeNoteNumbers.insert(m->getNoteNumber());
					}
					else {
						auto it = activeNoteNumbers.find(m->getNoteNumber());
						if (it != activeNoteNumbers.end())
							activeNoteNumbers.erase(it);
					}

					notifyListeners(e);
				}
			}
		}
	}
	this->samplePosition = position;
}

void Track::sendAllNotesOff()
{
	// Build a unique set so we send exactly one NoteOff per pitch,
	// even if the multiset contains duplicate entries for the same note.
	std::set<int> unique(activeNoteNumbers.begin(), activeNoteNumbers.end());
	for (int note : unique) {
		juce::MidiMessage noteOff = juce::MidiMessage::noteOff(midiChannel, note, (juce::uint8)0);
		Event* e = new Event();
		e->setMessage(noteOff);
		e->setType(Event::Type::GATE);
		e->setNote(note);
		e->setValue(0);
		notifyListeners(e);
	}
	activeNoteNumbers.clear();
}

int Track::getSamplePosition()
{
	if (samplePosition < 0)
		samplePosition = 0;
	return (samplePosition  / bufferSize) * bufferSize; 
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

			if (getBuffer()->getNumChannels() == 1) {
				return getBuffer()->getSample(0, sample - currentRegion->getSampleOffset());
			}
			else {
				return getBuffer()->getSample(channel, sample - currentRegion->getSampleOffset());
			}			
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
	drawGrid(g);
}

// Berechnet die Rasterposition in Samples
long Track::getGridSizeInSamples() const {
	double beatsPerSecond = bpm / 60.0;
	double samplesPerBeat = sampleRate / beatsPerSecond;

	switch (gridSize) {
	case WHOLE_NOTE:     return (long)(samplesPerBeat * 4.0);
	case HALF_NOTE:      return (long)(samplesPerBeat * 2.0);
	case QUARTER_NOTE:   return (long)(samplesPerBeat * 1.0);
	case EIGHTH_NOTE:    return (long)(samplesPerBeat * 0.5);
	case SIXTEENTH_NOTE: return (long)(samplesPerBeat * 0.25);
	default:             return (long)(samplesPerBeat * 1.0);
	}
}

// Berechnet die Rasterposition in Pixeln
int Track::getGridSizeInPixels() const {
	long samplesPerGrid = getGridSizeInSamples();
	return (int)(((double)samplesPerGrid * zoom) / sampleRate);
}

void Track::drawGrid(Graphics& g) {
	if (!gridEnabled) return;

	g.setColour(Colours::lightgrey.withAlpha(0.3f));

	long samplesPerGrid = getGridSizeInSamples();
	if (samplesPerGrid <= 0) return;

	// WICHTIG: Verwende exakte Multiplikation statt akkumulierte Addition
	int gridCount = (int)(getWidth() * sampleRate / (zoom * samplesPerGrid)) + 2;

	for (int i = 0; i <= gridCount; i++) {
		// Exakte Berechnung für jeden Grid-Punkt
		long exactSample = i * samplesPerGrid;
		double exactPixel = (double)exactSample * zoom / sampleRate;
		int pixelPos = (int)std::round(exactPixel);

		if (pixelPos >= 0 && pixelPos < getWidth()) {
			g.drawVerticalLine(pixelPos, 0, getHeight());
		}
	}
}

void Track::resized() {
	for (std::vector<Region*>::iterator it = regions.begin(); it != regions.end(); ++it) {
		(*it)->setSize((*it)->getWidth(), getHeight() - 5);
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

		if (r->getBuffer()->getNumChannels() == 1) {
			audioBuffer->copyFrom(0, sampleOffset, *r->getBuffer(), 0, 0, numSamples);
			audioBuffer->copyFrom(1, sampleOffset, *r->getBuffer(), 0, 0, numSamples);
		}
		else {
			audioBuffer->copyFrom(0, sampleOffset, *r->getBuffer(), 0, 0, numSamples);
			audioBuffer->copyFrom(1, sampleOffset, *r->getBuffer(), 1, 0, numSamples);
		}

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
