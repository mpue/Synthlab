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
#include "../Actions/SplitRegionAction.h"
#include "../Project/Project.h"
#include <iostream>
#include <limits>
#include <cstdlib>
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

		// Falls die Stelle bereits belegt ist, an die nächste freie Position rücken.
		newSampleOffset = findNextFreeOffset(nullptr, newSampleOffset, numSamplesToCopy);
		newX = (int)((double)newSampleOffset / sampleRate * zoom);

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
		// Falls die Stelle bereits belegt ist, an die nächste freie Position rücken.
		newOffset = findNextFreeOffset(nullptr, newOffset, mregion->getNumSamples());
		const int newX = (int)((double)newOffset / sampleRate * zoom);
		MidiRegion* clone = mregion->clone(newOffset);
		clone->setDragger(dragger);
		clone->setBounds(newX, 0, region->getWidth(), getHeight());
		clone->addChangeListener(this);

		this->regions.push_back(clone);
		this->currentRegion = clone;
		addAndMakeVisible(clone);
		clearSelection();
		dragger->setSelected(clone, true);
		repaint();
	}
}

Region* Track::pasteRegion(Region* source, long targetSampleOffset) {
	if (source == nullptr)
		return nullptr;

	if (AudioRegion* a = dynamic_cast<AudioRegion*>(source)) {
		if (type != AUDIO)
			return nullptr;

		const int numSamplesToCopy = a->getNumSamples();

		AudioRegion* duplicate = new AudioRegion(a, *manager, sampleRate, 0, numSamplesToCopy);
		duplicate->setDragger(dragger);

		long newSampleOffset = findNextFreeOffset(nullptr, targetSampleOffset, numSamplesToCopy);
		int newX = (int)((double)newSampleOffset / sampleRate * zoom);

		Rectangle<int>* bounds = new Rectangle<int>(0, 0, source->getWidth(), getHeight());
		duplicate->setBounds(newX, 0, source->getWidth(), source->getHeight());
		duplicate->setThumbnailBounds(bounds);
		duplicate->setLoopCount(0);

		this->regions.push_back(duplicate);
		this->currentRegion = duplicate;
		this->currentRegion->toFront(true);
		addAndMakeVisible(duplicate);

		clearSelection();
		dragger->setSelected(duplicate, true);

		duplicate->setSampleOffset(newSampleOffset, false, false);
		duplicate->setOffset(newX);
		duplicate->setClipRefId(a->getClipRefid());
		duplicate->addChangeListener(this);

		if (zoom > 0)
			duplicate->setZoom(zoom);

		// Mirror the duplicate's audio into the track's shared audioBuffer
		// at the new offset, matching what duplicateRegion does.
		if (duplicate->getBuffer()->getNumChannels() == 1) {
			audioBuffer->copyFrom(0, newSampleOffset, *duplicate->getBuffer(), 0, 0, numSamplesToCopy);
			audioBuffer->copyFrom(1, newSampleOffset, *duplicate->getBuffer(), 0, 0, numSamplesToCopy);
		}
		else {
			audioBuffer->copyFrom(0, newSampleOffset, *duplicate->getBuffer(), 0, 0, numSamplesToCopy);
			audioBuffer->copyFrom(1, newSampleOffset, *duplicate->getBuffer(), 1, 0, numSamplesToCopy);
		}

		repaint();
		return duplicate;
	}

	if (MidiRegion* m = dynamic_cast<MidiRegion*>(source)) {
		if (type != MIDI && type != INSTRUMENT)
			return nullptr;

		long newOffset = findNextFreeOffset(nullptr, targetSampleOffset, m->getNumSamples());
		const int newX = (int)((double)newOffset / sampleRate * zoom);

		MidiRegion* clone = m->clone(newOffset);
		clone->setDragger(dragger);
		clone->setBounds(newX, 0, source->getWidth(), getHeight());
		clone->addChangeListener(this);

		// Cross-Track-Paste: Quell-Region kann auf einem anderen MIDI-Kanal
		// liegen. Auf den Kanal des Ziel-Tracks umschreiben — TrackIn filtert
		// eingehende Events sonst und die kopierte Region bliebe stumm.
		clone->setMidiChannelForAllMessages(midiChannel);

		this->regions.push_back(clone);
		this->currentRegion = clone;
		addAndMakeVisible(clone);

		clearSelection();
		dragger->setSelected(clone, true);

		if (zoom > 0)
			clone->setZoom(zoom);

		repaint();
		return clone;
	}

	return nullptr;
}

void Track::pasteRegions(long startSampleOffset) {
	const auto& clipboard = Session::getInstance()->getRegionsClipboard();
	if (clipboard.empty())
		return;

	long cursor = startSampleOffset;
	clearSelection();

	for (Region* source : clipboard) {
		Region* placed = pasteRegion(source, cursor);
		if (placed != nullptr) {
			// Stack subsequent pastes after the previous one so multi-region
			// pastes stay sequential instead of stomping the same offset.
			cursor = placed->getSampleOffset() + placed->getNumSamples();
		}
	}
}

Region* Track::addRegion(String refId, File file, double sampleRate, long sampleOffset) {
	AudioRegion* region = new AudioRegion(file, refId, *manager, sampleRate);
	region->setDragger(dragger);
	Rectangle<int>* bounds = new Rectangle<int>(0, 0, region->getWidth(), getHeight());
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
	Rectangle<int>* bounds = new Rectangle<int>(0, 0, region->getWidth(), getHeight());
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
	Rectangle<int>* bounds = new Rectangle<int>(0, 0, region->getWidth(), getHeight());
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

	// Match the track's current zoom — without this the new region keeps the
	// MidiRegion constructor default (40), so on a track at any other zoom it
	// renders at the wrong x and width and visually overlaps the previously
	// recorded region. Newly-recorded notes then appear to land "inside" the
	// old region.
	if (zoom > 0)
		region->setZoom(zoom);

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

void Track::attachRegion(Region* r)
{
    r->addChangeListener(this);
    r->setDragger(dragger);
    addAndMakeVisible(r);
    regions.push_back(r);
    if (zoom > 0)
        r->setZoom(zoom);
}

void Track::detachRegion(Region* r)
{
    r->removeAllChangeListeners();
    removeChildComponent(r);
    if (dragger->isSelected(r))
        dragger->setSelected(r, false);
    auto it = std::find(regions.begin(), regions.end(), r);
    if (it != regions.end())
        regions.erase(it);
}

void Track::splitRegion()
{
    long sampleNum = getSamplePosition();

    Region* baseRegion = getRegionAt(sampleNum);
    if (!baseRegion || !baseRegion->isSelected())
        return;

    if (AudioRegion* region = dynamic_cast<AudioRegion*>(baseRegion)) {
        long numLeftSamples  = sampleNum - region->getSampleOffset();
        long numRightSamples = region->getNumSamples() - numLeftSamples;

        AudioRegion* leftRegion  = new AudioRegion(region, *manager, sampleRate, 0, numLeftSamples);
        AudioRegion* rightRegion = new AudioRegion(region, *manager, sampleRate, numLeftSamples, numRightSamples);

        Rectangle<int>* leftBounds  = new Rectangle<int>(0, 0, leftRegion->getThumbnail()->getTotalLength() * 20, getHeight());
        Rectangle<int>* rightBounds = new Rectangle<int>(0, 0, rightRegion->getThumbnail()->getTotalLength() * 20, getHeight());

        leftRegion->setBounds(region->getX(), 0, leftRegion->getWidth(), getHeight());
        leftRegion->setThumbnailBounds(leftBounds);

        rightRegion->setBounds(markerPosition, 0, rightRegion->getWidth(), getHeight());
        rightRegion->setThumbnailBounds(rightBounds);

        leftRegion->setSampleOffset(region->getSampleOffset(), false, false);
        leftRegion->setOffset(region->getOffset());

        rightRegion->setSampleOffset(sampleNum, false, false);
        rightRegion->setOffset((double)sampleNum / sampleRate);

        Project::getInstance()->getUndoManager()->beginNewTransaction();
        Project::getInstance()->getUndoManager()->perform(
            new SplitRegionAction(this, region, leftRegion, rightRegion));
        return;
    }

    if (MidiRegion* region = dynamic_cast<MidiRegion*>(baseRegion)) {
        auto [leftRegion, rightRegion] = region->splitAt(sampleNum, dragger);
        if (!leftRegion || !rightRegion)
            return;

        const int leftX = region->getX();
        const int rightX = (int)((sampleNum / sampleRate) * zoom);
        const int leftW  = (int)((leftRegion->getNumSamples()  / sampleRate) * zoom);
        const int rightW = (int)((rightRegion->getNumSamples() / sampleRate) * zoom);

        leftRegion->setBounds(leftX,  0, leftW,  getHeight());
        rightRegion->setBounds(rightX, 0, rightW, getHeight());

        Project::getInstance()->getUndoManager()->beginNewTransaction();
        Project::getInstance()->getUndoManager()->perform(
            new SplitRegionAction(this, region, leftRegion, rightRegion));
    }
}

void Track::dispatchMidiMessages(const std::vector<juce::MidiMessage*>& messages)
{
	for (auto* m : messages) {
		if (m == nullptr)
			continue;

		Event* e = new Event();
		e->setMessage(*m);
		e->setType(Event::Type::GATE);
		e->setNote(m->getNoteNumber());
		if (m->getVelocity() > 127)
			m->setVelocity(127);
		e->setValue(m->isNoteOn() ? m->getVelocity() : 0);

		// Track which notes are currently on. find+erase(iterator) makes one
		// NoteOff remove exactly one entry from the multiset — important when
		// two NoteOns for the same pitch overlap (duplicate notes in piano roll).
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

void Track::setSamplePosition(int position, bool dispatchEvents)
{
	if ((type == MIDI || type == INSTRUMENT) && position != samplePosition) {

		const int bufferSize = Project::getInstance()->getBufferSize();

		MidiRegion* r = dynamic_cast<MidiRegion*>(getRegionAt(position));

		// If the active region changed (ended, seeked, or looped back),
		// stop all currently playing notes to prevent stuck notes.
		if (r != lastActiveRegion && !activeNoteNumbers.empty()) {
			sendAllNotesOff();
		}
		lastActiveRegion = r;

		// Silent seek: update samplePosition and lastActiveRegion (so
		// getCurrentRegion etc. still work) but don't fire any MIDI. Used
		// when the user moves the playhead while the transport is stopped.
		if (!dispatchEvents) {
			this->samplePosition = position;
			return;
		}

		if (r != nullptr) {
			// Use exact bufferSize window – bufferSize*2 caused each message to
			// appear in two consecutive windows, firing Note-Ons twice but
			// Note-Offs only once, resulting in stuck notes.
			dispatchMidiMessages(r->getMessages(position - r->getSampleOffset(), bufferSize));
		}

		// If the upcoming block straddles the boundary into the next region,
		// also dispatch the leading events of that next region. Otherwise events
		// at the very start of an abutting region (e.g. a NoteOn at relative
		// time 0) get skipped because the block window in the current region
		// can't extend past the region's end, and the next call to
		// setSamplePosition starts the window past the region's beginning.
		const long blockEnd = (long)position + bufferSize;
		if (r != nullptr) {
			const long rEnd = r->getSampleOffset() + r->getNumSamples();
			if (rEnd > position && rEnd < blockEnd) {
				MidiRegion* nextR = dynamic_cast<MidiRegion*>(getRegionAt(rEnd));
				if (nextR != nullptr && nextR != r) {
					if (!activeNoteNumbers.empty())
						sendAllNotesOff();
					lastActiveRegion = nextR;
					const long leadLen = blockEnd - rEnd;
					dispatchMidiMessages(nextR->getMessages(0, (int)leadLen));
				}
			}
		}
		else {
			// Currently between regions. If a region begins inside this block,
			// dispatch its head so its first event isn't missed.
			for (Region* base : regions) {
				MidiRegion* mr = dynamic_cast<MidiRegion*>(base);
				if (mr == nullptr) continue;
				const long rStart = mr->getSampleOffset();
				if (rStart > position && rStart < blockEnd) {
					if (!activeNoteNumbers.empty())
						sendAllNotesOff();
					lastActiveRegion = mr;
					const long leadLen = blockEnd - rStart;
					dispatchMidiMessages(mr->getMessages(0, (int)leadLen));
					break;
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

void Track::forcePositionReset()
{
	// Set samplePosition to -1 so the next setSamplePosition call always
	// dispatches MIDI events, even when the new position is 0.
	// Used before bounce/export to ensure notes at t=0 are not skipped.
	samplePosition = -1;
	activeNoteNumbers.clear();
	lastActiveRegion = nullptr;
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

// Clamps `desiredOffset` so [desiredOffset, desiredOffset+length) does not
// intersect any other region on this track. If the desired range overlaps
// a neighbour, the offset is pushed up against that neighbour's edge (the
// closer one). If no valid placement exists, returns the moving region's
// current offset (i.e. revert).
long Track::clampOffsetForRegion(Region* moving, long desiredOffset, long length) const {
	if (length <= 0)
		return desiredOffset;
	if (desiredOffset < 0)
		desiredOffset = 0;

	long minStart = 0;
	long maxStart = std::numeric_limits<long>::max();

	for (Region* r : regions) {
		if (r == nullptr || r == moving)
			continue;
		const long rStart = r->getSampleOffset();
		const long rEnd   = rStart + r->getNumSamples();
		if (rEnd <= rStart)
			continue;

		if (rEnd <= desiredOffset) {
			// neighbour entirely to the left
			if (rEnd > minStart) minStart = rEnd;
		}
		else if (rStart >= desiredOffset + length) {
			// neighbour entirely to the right
			const long candidate = rStart - length;
			if (candidate < maxStart) maxStart = candidate;
		}
		else {
			// overlap with neighbour — push to whichever side is closer
			const long pushLeftOffset  = rEnd;
			const long pushRightOffset = rStart - length;
			const long distLeft  = std::abs(pushLeftOffset  - desiredOffset);
			const long distRight = std::abs(pushRightOffset - desiredOffset);
			if (distLeft <= distRight) {
				if (rEnd > minStart) minStart = rEnd;
			}
			else {
				if (pushRightOffset < maxStart) maxStart = pushRightOffset;
			}
		}
	}

	if (minStart > maxStart) {
		// no room — revert
		return moving != nullptr ? moving->getSampleOffset() : desiredOffset;
	}

	long clamped = desiredOffset;
	if (clamped < minStart) clamped = minStart;
	if (clamped > maxStart) clamped = maxStart;
	if (clamped < 0) clamped = 0;
	return clamped;
}

// Returns the maximum length the region can have at `offset` without
// running into the next region on the right.
long Track::clampLengthForRegion(Region* moving, long offset, long desiredLength) const {
	long maxLen = desiredLength;
	for (Region* r : regions) {
		if (r == nullptr || r == moving)
			continue;
		const long rStart = r->getSampleOffset();
		if (rStart >= offset) {
			const long allowed = rStart - offset;
			if (allowed < maxLen) maxLen = allowed;
		}
	}
	if (maxLen < 0) maxLen = 0;
	return maxLen;
}

// First sample offset >= desiredOffset where a region of `length` samples
// fits without overlap. Walks past blocking neighbours sequentially.
long Track::findNextFreeOffset(Region* moving, long desiredOffset, long length) const {
	long candidate = desiredOffset < 0 ? 0 : desiredOffset;
	bool moved = true;
	while (moved) {
		moved = false;
		for (Region* r : regions) {
			if (r == nullptr || r == moving)
				continue;
			const long rStart = r->getSampleOffset();
			const long rEnd   = rStart + r->getNumSamples();
			if (rEnd <= rStart)
				continue;
			if (candidate < rEnd && candidate + length > rStart) {
				candidate = rEnd;
				moved = true;
				break;
			}
		}
	}
	return candidate;
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
	// Trust currentRegion to already point at the recording region — set by
	// addRegion() in TrackNavigator::setRecording. The previous lookup here
	// (getRegionAt(sample)) treated the audio sample VALUE as a timeline
	// position, which collapsed every write onto the region at sample 0
	// (typically the first recorded region) regardless of where the new
	// recording actually started.
	if (currentRegion != nullptr) {
		AudioSampleBuffer* buf = getBuffer();
		if (buf != nullptr) {
			if (sampleNum >= buf->getNumSamples()) {
				AudioRegion* ar = dynamic_cast<AudioRegion*>(currentRegion);
				if (ar == nullptr)
					return;  // not an AudioRegion — refuse to write
				// TODO : from card
				ar->resizeAudio(256);
			}
			buf = getBuffer(); // reassign, because of resizing
			if (buf == nullptr || sampleNum >= buf->getNumSamples())
				return;
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
