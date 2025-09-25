/*
  ==============================================================================

	WaveNavigator.cpp
	Created: 24 Nov 2016 8:04:54am
	Author:  Matthias Pueski

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "TrackNavigator.h"
#include "AudioRegion.h"
#include "MidiRegion.h"
#include "../Project/Project.h"
#include "../Session.h"
#include "../Mixer.h"
#include <iterator>
#include <iostream>
#include "../ExtendedFileBrowser.h"
#include "../PianoRollComponent.h"
#include "../MidiRegionEventEditor.h"
#include "../PluginModule.h"
#include "../MidiProcessor.h"

using namespace std;

//==============================================================================
TrackNavigator::TrackNavigator(AudioDeviceManager* deviceManager, Viewport* view, PropertyView* propertyView, TransportDisplayComponent* transport)
{
	this->transportDisplay = transport;
	this->propertyView = propertyView;
	this->timeLine = std::make_unique<TimeLine>(Project::getInstance()->getTrackLength());
	this->timeLine->setBounds(0, 0, getWidth(), timelineOffset);
	this->timeLine->setSize(getWidth(), timelineOffset);
	this->timeLine->setTempoAndTimeSignature(Project::getInstance()->getTempo(),4,4);
	addAndMakeVisible(timeLine.get());
	this->deviceManager = deviceManager;
	this->selector = std::make_unique<WaveSelector>();
	this->selector->addChangeListener(this);
	this->selector->setSize(getWidth(), getHeight());
	this->selector->setBounds(0, 0, getWidth(), getHeight());
	addAndMakeVisible(selector.get());
	this->zoom = 40.0f;
	int64 lengthInSamples = Project::getInstance()->getTrackLength() * Project::getInstance()->getSampleRate();
	this->marker = new PositionMarker(lengthInSamples);
	this->marker->setSampleRate(Project::getInstance()->getSampleRate());
	marker->setZoom(zoom); // Zoom gleich mitgeben	
	this->position = 0;
	this->dragger = new MultiComponentDragger();
	
	double beatsPerSecond = Project::getInstance()->getTempo() / 60.0;
	double samplesPerBeat = Project::getInstance()->getSampleRate() / beatsPerSecond;
	int raster = (int)(samplesPerBeat * zoom) / Project::getInstance()->getSampleRate(); // Viertelnoten
	dragger->setRaster(raster);

	Project::getInstance()->addChangeListener(this);
	addChangeListener(Mixer::getInstance());
	addAndMakeVisible(marker);
	marker->setAlwaysOnTop(true);
	marker->setBounds(0, 0, 2, getHeight());
	this->view = view;
	view->getHorizontalScrollBar().addListener(this);
	setWantsKeyboardFocus(true);
	setMouseClickGrabsKeyboardFocus(true);
	setInterceptsMouseClicks(true, true);

	timeLine->onLoopChanged = [this](double start, double end, bool enabled) {
		this->loopStart = start;
		this->loopEnd = end;
		this->loopEnabled = enabled;		
	};

	transportDisplay->onToggleLoop = [this]() {

		bool loopState = transportDisplay->isLoopActive();
		this->timeLine->setLoopEnabled(loopState);
		this->loopEnabled = loopState;
	};

}

TrackNavigator::~TrackNavigator()
{
	timeLine = nullptr;
	selector = nullptr;

	for (std::vector<Track*>::iterator it = tracks.begin(); it != tracks.end(); ++it) {
		delete* it;
	}

	delete dragger;
	delete marker;

}

void TrackNavigator::paint(Graphics& g)
{
	marker->paint(g);
}

void TrackNavigator::clearTracks() {
	for (std::vector<Track*>::iterator it = tracks.begin(); it != tracks.end(); ++it) {
		delete* it;
	}
	tracks.clear();
}

void TrackNavigator::eventReceived(Event* e)
{
}

void TrackNavigator::allNotesOff() {
	for (int i = 0; i < Project::getInstance()->getRoot()->getModules()->size(); i++) {

		for (int j = 0; j < 16; j++) {
			Project::getInstance()->getMidiProcessor()->sendMidiStopMessage(Project::getInstance()->getRoot()->getModules()->at(i), j);

			for (int k = 0; k < 128; k++) {
				Project::getInstance()->getMidiProcessor()->sendGateMessage(Project::getInstance()->getRoot()->getModules()->at(i), j, 0, k, false);
			}
		}
	}
}

float TrackNavigator::getZoom()
{
	return this->zoom;
}

void TrackNavigator::setZoom(float zoom) {
	this->zoom = zoom;
	for (int i = 0; i < tracks.size(); i++) {
		tracks.at(i)->setZoom(zoom);
	}
	marker->setZoom(zoom);
	double beatsPerSecond = Project::getInstance()->getTempo() / 60.0;
	double samplesPerBeat = Project::getInstance()->getSampleRate() / beatsPerSecond;
	int raster = (int)(samplesPerBeat * zoom) / Project::getInstance()->getSampleRate(); // Viertelnoten
	dragger->setRaster(raster);
	

	// constrainer.set
	// (this->zoom / 4);
	// constrainer.setMaxX(getMaxLength() * this->zoom);
}

void TrackNavigator::updateTrackLayout(ChangeBroadcaster* source)
{
	int index = -1;

	for (int i = 0; i < tracks.size(); i++) {
		if (source == tracks.at(i) && i < tracks.size() - 1) {
			index = i;
			break;
		}
	}

	if (index >= 0 && (int)tracks.size() > index + 1) {

		for (int i = index + 1; i < tracks.size(); i++) {
			if (i == 0)
				tracks.at(i)->setTopLeftPosition(tracks.at(i - 1)->getX(), tracks.at(i - 1)->getPosition().getY() + tracks.at(i - 1)->getHeight() + timelineOffset);
			else
				tracks.at(i)->setTopLeftPosition(tracks.at(i - 1)->getX(), tracks.at(i - 1)->getPosition().getY() + tracks.at(i - 1)->getHeight());
		}

	}
	else {

		if (tracks.size() == 1) {
			tracks.at(0)->setTopLeftPosition(0, timelineOffset);
		}

	}

	adjustHeight();
	sendChangeMessage();
}

void TrackNavigator::adjustHeight()
{
	int height = 0;

	for (int i = 0; i < tracks.size(); i++) {
		height += tracks.at(i)->getHeight();
	}

	this->marker->setBounds(0, 0, 2, getHeight());

	setSize(getWidth(), height + timelineOffset);
}

bool TrackNavigator::isInterestedInDragSource(const SourceDetails& dragSourceDetails)
{
	juce::String name = dragSourceDetails.description.toString();
	if (name.endsWith(".wav")) {
		return true;
	}

	return false;
}

void TrackNavigator::itemDropped(const SourceDetails& dragSourceDetails)
{
	for (int i = 0; i < tracks.size(); i++) {
		if (tracks.at(i)->isSelected()) {
			juce::File file = juce::File(dragSourceDetails.description.toString());
			tracks.at(i)->addRegion(file.getFileNameWithoutExtension(), file, Project::getInstance()->getSampleRate());
			Project::getInstance()->addAudioFile(file.getFileNameWithoutExtension(), file.getFullPathName());
			repaint();
		}
	}

	Logger::getCurrentLogger()->writeToLog(dragSourceDetails.description.toString());
}

void TrackNavigator::timerCallback()
{
	repaint();
}

ValueTree TrackNavigator::getConfiguration()
{
	ValueTree v("Tracks");

	for (int i = 0; i < tracks.size(); i++) {

		ValueTree track("Track");
		track.setProperty("name", tracks.at(i)->getName(), nullptr);

		// Setze den Typ unabhängig von den Regionen
		if (tracks.at(i)->getType() == Track::AUDIO) {
			track.setProperty("type", "audio", nullptr);
		}
		else if (tracks.at(i)->getType() == Track::INSTRUMENT) {
			track.setProperty("type", "instrument", nullptr);
			track.setProperty("midiChannel", tracks.at(i)->getMidiChannel(), nullptr);
			track.setProperty("plugin", tracks.at(i)->getInstrumentName(), nullptr);
		}
		else {
			track.setProperty("type", "unknown", nullptr); // Fallback
		}

		ValueTree regions("Regions");

		for (int j = 0; j < tracks.at(i)->getRegions().size(); j++) {

			ValueTree region("Region");
			region.setProperty("name", tracks.at(i)->getRegions().at(j)->getName(), nullptr);

			if (tracks.at(i)->getType() == Track::AUDIO) {
				if (auto* ar = dynamic_cast<AudioRegion*>(tracks.at(i)->getRegions().at(j))) {
					region.setProperty("clipRefId", ar->getClipRefid(), nullptr);
					region.setProperty("sampleOffset", (int)ar->getSampleOffset(), nullptr);
					region.setProperty("numSamples", ar->getNumSamples(), nullptr);
					region.setProperty("width", ar->getWidth(), nullptr);
				}
			}
			else if (tracks.at(i)->getType() == Track::MIDI || tracks.at(i)->getType() == Track::INSTRUMENT) {
				if (auto* mr = dynamic_cast<MidiRegion*>(tracks.at(i)->getRegions().at(j))) {
					region.setProperty("sampleOffset", (int)mr->getSampleOffset(), nullptr);
					region.setProperty("numSamples", mr->getNumSamples(), nullptr);
					region.setProperty("width", mr->getWidth(), nullptr);

					ValueTree events("Events");

					for (int k = 0; k < mr->getNumMessages(); k++) {
						if (auto* msg = mr->getMessageAt(k)) {
							ValueTree e("Event");

							if (msg->isNoteOn()) {
								e.setProperty("type", "noteon", nullptr);
								e.setProperty("note", msg->getNoteNumber(), nullptr);
							}
							else if (msg->isNoteOff()) {
								e.setProperty("type", "noteoff", nullptr);
								e.setProperty("note", msg->getNoteNumber(), nullptr);
							}
							else if (msg->isController()) {
								e.setProperty("type", "control", nullptr);
								e.setProperty("controller", msg->getControllerNumber(), nullptr);
								e.setProperty("value", msg->getControllerValue(), nullptr);
							}
							else if (msg->isPitchWheel()) {
								e.setProperty("type", "pitch", nullptr);
								e.setProperty("pitch", msg->getPitchWheelValue(), nullptr);
							}

							e.setProperty("channel", msg->getChannel(), nullptr);
							e.setProperty("velocity", msg->getVelocity(), nullptr);
							e.setProperty("timestamp", msg->getTimeStamp(), nullptr);

							events.addChild(e, -1, nullptr);
						}
					}

					region.addChild(events, -1, nullptr);
				}
			}

			regions.addChild(region, -1, nullptr);
		}

		track.addChild(regions, -1, nullptr);
		v.addChild(track, -1, nullptr);
	}

	return v;
}

void TrackNavigator::setConfiguration(ValueTree* config)
{
}

void TrackNavigator::scrollBarMoved(ScrollBar* scrollBarThatHasMoved, double newRangeStart)
{
	scrollbarOffset = newRangeStart;
}

double TrackNavigator::getPosition() {
	return position;
}

void TrackNavigator::setPosition(double position) {
	stopTimer();
	//int pos = (int)position - (int)position % 2;
	this->position = position;
	this->samplePosition = position * Project::getInstance()->getSampleRate();

	// Align position marker to buffer size
	int bufferSize = Project::getInstance()->getBufferSize();
	this->samplePosition = ((int)(this->samplePosition + bufferSize / 2) / bufferSize) * bufferSize;

	Logger::getCurrentLogger()->writeToLog("samplePosition  : " + String(samplePosition));
	std::function<void(void)> changeLambda =
		[=]() {
		marker->setSamplePosition(samplePosition);
		transportDisplay->setTransportState(samplePosition, Project::getInstance()->getSampleRate(), Project::getInstance()->getTempo(), 4, 4);
		};
	juce::MessageManager::callAsync(changeLambda);
	startTimer(50);
}

int TrackNavigator::getSamplePosition() {
	return samplePosition;
}

void TrackNavigator::ResetMarkerPosition()
{
	setPosition(0);
	for (int i = 0; i < tracks.size(); i++) {
		tracks.at(i)->setSamplePosition(0);
		tracks.at(i)->setCurrentMarkerPosition(0);
	}
	sendChangeMessage();

}

void TrackNavigator::setSamplePosition(int position) {

	// Logger::getCurrentLogger()->writeToLog("setSamplePosition : " + String(position));
	this->samplePosition = position;

	if (loopEnabled) {
		if (this->samplePosition > loopEnd * Project::getInstance()->getSampleRate()) {
			this->samplePosition = loopStart * Project::getInstance()->getSampleRate();
		}
	}

	for (int i = 0; i < tracks.size(); i++) {
		tracks.at(i)->setSamplePosition(this->getSamplePosition());
	}
	this->position = (float)samplePosition / Project::getInstance()->getSampleRate();
	std::function<void(void)> changeLambda =
		[=]() {
		marker->setSamplePosition(this->samplePosition);
		transportDisplay->setTransportState(samplePosition, Project::getInstance()->getSampleRate(), Project::getInstance()->getTempo(), 4, 4);
		if (pianoRoll != nullptr) {
			pianoRoll->setPlaybackPosition(this->samplePosition);

		}
		};
	juce::MessageManager::callAsync(changeLambda);
}

double TrackNavigator::getMaxLength() {
	double maxlen = 0;

	for (int i = 0; i < tracks.size(); i++) {

		double len = tracks.at(i)->getMaxLength();

		if (len > maxlen) {
			maxlen = len;
		}
	}

	return maxlen;
}

void TrackNavigator::setPlaying(bool playing) {
	this->playing = playing;
	Session::getInstance()->setRecordingStartTime();
	for (int i = 0; i < tracks.size(); i++) {
		for (int j = 0; j < tracks.at(i)->getRegions().size(); j++) {
			tracks.at(i)->getRegions().at(j)->reset();
			tracks.at(i)->setSamplePosition(position);
			tracks.at(i)->setCurrentMarkerPosition(position);
		}
	}
	for (int i = 0; i < Project::getInstance()->getRoot()->getModules()->size(); i++) {

		PluginModule* module = nullptr;

		if ((module = dynamic_cast<PluginModule*>(Project::getInstance()->getRoot()->getModules()->at(i))) != nullptr) {
			module->getPlayHead().setRecording(false);
			module->getPlayHead().setPlaying(playing);
		}
	}

	sendChangeMessage();
}

bool TrackNavigator::isPlaying() {
	return playing;
}

void TrackNavigator::noteOffForAll() {


}

void TrackNavigator::setRecording(bool recording) {

	for (int i = 0; i < Project::getInstance()->getRoot()->getModules()->size(); i++) {

		PluginModule* module = nullptr;

		// make sure we do not send this event to a plugin module

		if ((module = dynamic_cast<PluginModule*>(Project::getInstance()->getRoot()->getModules()->at(i))) != nullptr) {
			module->getPlayHead().setRecording(recording);
			module->getPlayHead().setPlaying(false);
		}
	}

	if (recording) {
		noteOffForAll();
		startTimer(50);
		Session::getInstance()->setRecordingStartTime();
		recordStart = (long)marker->getSamplePosition();
		if (recordStart < 0) {
			recordStart = 0;
		}
		for (int i = 0; i < tracks.size(); i++) {
			if (tracks.at(i)->isRecording()) {

				if (tracks.at(i)->getType() == Track::Type::MIDI || tracks.at(i)->getType() == Track::Type::INSTRUMENT) {

					MidiRegion* r = dynamic_cast<MidiRegion*>(recordingRegion);
					if (r == nullptr) {
						recordingRegion = tracks.at(i)->addMidiRegion(Project::getInstance()->getSampleRate(), recordStart, Project::getInstance()->getSampleRate() * 10);
					}
					recordingRegion->startRecording();
				}
				else if (tracks.at(i)->getType() == Track::Type::AUDIO) {
					AudioRegion* r = dynamic_cast<AudioRegion*>(tracks.at(i)->getRegionAt(recordStart));
					if (r == nullptr) {
						r = tracks.at(i)->addRegion(nullptr, Project::getInstance()->getSampleRate(), recordStart, Project::getInstance()->getSampleRate() * 10);
					}
					r->startRecording();
				}

			}
		}
	}
	else {
		stopTimer();
		recordingRegion = nullptr;
		Session::getInstance()->setRecordingStopTime();
		recordStop = marker->getSamplePosition() * Project::getInstance()->getSampleRate();

		for (int i = 0; i < tracks.size(); i++) {
			if (tracks.at(i)->isRecording()) {
				if (tracks.at(i)->getType() == Track::Type::MIDI || tracks.at(i)->getType() == Track::Type::INSTRUMENT) {
					for (int j = 0; j < tracks.at(i)->getRegions().size(); j++) {

						MidiRegion* mr = dynamic_cast<MidiRegion*>(recordingRegion);

						if (mr != nullptr) {
							mr->stopRecording();
							mr->sanitize();
						}
					}
				}
				else if (tracks.at(i)->getType() == Track::Type::AUDIO) {
					for (int j = 0; j < tracks.at(i)->getRegions().size(); j++) {

						AudioRegion* ar = dynamic_cast<AudioRegion*>(tracks.at(i)->getRegions().at(j));

						if (ar != nullptr) {
							ar->stopRecording();
						}
					}
				}
			}
		}
	}

	this->recording = recording;
}

bool TrackNavigator::isRecording() {
	return recording;
}

std::vector<Track*> TrackNavigator::getTracks() {
	return this->tracks;
}

StringArray TrackNavigator::getTrackList()
{
	StringArray trackList = StringArray();

	for (int i = 0; i < tracks.size(); i++) {
		trackList.add(tracks[i]->getName());
	}

	return trackList;
}

void TrackNavigator::openPianoRoll(MidiRegion* r)
{
	if (pianoRoll != nullptr) {
		pianoRoll = nullptr;
	}
	pianoRoll = std::make_unique<PianoRollComponent>(r, Project::getInstance()->getSampleRate());

	pianoRoll->setGridResolution(16);

	Project::getInstance()->getSupplemental()->addTab("Piano roll", Colours::darkgrey, pianoRoll.get(), false);
	Project::getInstance()->getSupplemental()->setCurrentTabIndex(Project::getInstance()->getSupplemental()->getNumTabs() - 1);
}

void TrackNavigator::openEventEditor(MidiRegion* r)
{
	if (pianoRoll != nullptr) {
		pianoRoll = nullptr;
	}
	auto* eventEditor = new MidiRegionEventEditor(r, Project::getInstance()->getSampleRate());

	Project::getInstance()->getSupplemental()->addTab("Event editor", Colours::darkgrey, eventEditor, true);
	Project::getInstance()->getSupplemental()->setCurrentTabIndex(Project::getInstance()->getSupplemental()->getNumTabs() - 1);
}


Track* TrackNavigator::addTrack(TrackConfig* tc) {

	Track* track = nullptr;

	if (tc->getType() == "audio") {
		track = new Track(Track::Type::AUDIO, Project::getInstance()->getSampleRate(), this->dragger);
	}
	else {
		track = new Track(Track::Type::MIDI, Project::getInstance()->getSampleRate(), this->dragger);
	}

	track->setName(tc->getName());
	track->setMidiOutputDevice(tc->getMidiInputDevice());
	track->setMidiInputDevice(tc->getMidiInputDevice());
	track->setInputChannels(tc->getInputChannels());
	track->setOutputChannels(tc->getOutputChannels());
	track->setMute(tc->isMute());
	track->setSolo(tc->isSolo());
	track->setMono(tc->isMono());
	track->setVolume(tc->getVolume());
	track->setGain(1.0f);
	track->setPan(tc->getPan());
	track->setMidiChannel(tc->getMidiChannel(), false);
	track->setHeight(tc->getHeight());

	addTrack(track);

	int height = 1;

	for (int i = 0; i < this->tracks.size(); i++) {
		height += tracks.at(i)->getHeight();
	}

	setBounds(getX(), getY(), getMaxLength() * this->zoom, height);

	this->marker->setSize(2, height);
	this->marker->setSampleLength(getMaxLength() * 300);
	this->selector->setSize(getWidth(), getHeight());
	this->timeLine->setBounds(0, 0, getWidth(), timelineOffset);
	this->timeLine->setSize(getWidth(), timelineOffset);

	repaint();
	sendChangeMessage();

	return track;
}

void TrackNavigator::addTrack(Track* track) {

	track->setNavigator(this);

	// deselect all tracks before adding a new one

	for (int i = 0; i < tracks.size(); i++) {
		tracks.at(i)->setSelected(false);
	}

	// calculate y position of new track

	int yPos = timelineOffset;

	for (int i = 0; i < this->tracks.size(); i++) {
		yPos += tracks.at(i)->getHeight();
	}

	// set bounds accordingly

	Project* p = Project::getInstance();
	track->setBounds(0, yPos, p->getTrackLength() * this->zoom, track->getHeight());

	this->tracks.push_back(track);
	this->currentTrack = track;

	addAndMakeVisible(track);

	track->addChangeListener(this);

	Mixer::getInstance()->addTrack(track);

	track->setSamplePosition(samplePosition);

	if (zoom > 0)
		track->setZoom(zoom);

	track->setSelected(true);
	propertyView->selectionChanged(track);
	this->selector->toFront(false);
	marker->setZoom(zoom);

	repaint();
}

Track* TrackNavigator::addTrack(Track::Type type, double sampleRate) {

	Track* track = new Track(type, sampleRate, this->dragger);
	addTrack(track);

	int height = timelineOffset;

	for (int i = 0; i < this->tracks.size(); i++) {
		height += tracks.at(i)->getHeight();
	}

	setBounds(getX(), getY(), getMaxLength() * this->zoom, height);

	this->marker->setSize(2, getHeight());
	this->marker->setSampleLength(getMaxLength() * 300);
	this->selector->setSize(getWidth(), getHeight());
	this->timeLine->setBounds(0, 0, getWidth(), timelineOffset);
	this->timeLine->setSize(getWidth(), timelineOffset);

	track->setVolume(1.0f);

	repaint();
	sendChangeMessage();

	return track;
}

void TrackNavigator::removeSelectedTrack()
{
	for (std::vector<Track*>::iterator it = tracks.begin(); it != tracks.end();) {
		if ((*it)->isSelected()) {
			delete* it;
			it = tracks.erase(it);
			break;
		}
		else {
			++it;
		}
	}

	if (tracks.size() > 0) {

		tracks.at(0)->setTopLeftPosition(tracks.at(0)->getX(), 0);

		for (int i = 1; i < tracks.size(); i++) {
			tracks.at(i)->setTopLeftPosition(tracks.at(i - 1)->getX(), tracks.at(i - 1)->getHeight() + tracks.at(i - 1)->getY());
		}
	}

	adjustHeight();
	repaint();
	sendChangeMessage();

}

Track* TrackNavigator::getCurrentTrack()
{
	return this->currentTrack;
}

void TrackNavigator::resized()
{
	int numOfTracks = (int)tracks.size();

	if (numOfTracks == 0) {
		numOfTracks = 1;
	}

	this->marker->setSize(2, getHeight());
	this->marker->setDrawingBounds(0, 25, getWidth(), getHeight());
	this->timeLine->setBounds(0, 0, getWidth(), timelineOffset);

}

WaveSelector* TrackNavigator::getSelector() {
	return this->selector.get();
}

void TrackNavigator::changeListenerCallback(ChangeBroadcaster* source)
{
	updateTrackLayout(source);

	if (source == Project::getInstance()) {
		tracklength = Project::getInstance()->getTrackLength();

		for (int i = 0; i < tracks.size(); i++) {
			tracks.at(i)->setTrackLength(tracklength);
		}
	}
	if (Track* track = dynamic_cast<Track*>(source)) {
		sendChangeMessage();
	}

}

bool TrackNavigator::keyPressed(const KeyPress& key, Component* originatingComponent) {

	if (key == KeyPress::spaceKey) {
		if (isPlaying())
		{
			setPlaying(false);
			stopTimer();
		}
		else
		{
			setPlaying(true);
			startTimer(50);
		}

	}
	if (key.getTextCharacter() == '+') {
		if (getMaxLength() == 0) {
			return false;
		}
		this->zoom += Project::getInstance()->getTempo() / 8;
		setSize(getMaxLength() * this->zoom, getHeight());
		setZoom(zoom);
		repaint();
		sendChangeMessage();
	}
	else if (key.getTextCharacter() == '-') {
		if (getMaxLength() == 0) {
			return false;
		}
		this->zoom -= Project::getInstance()->getTempo() / 8;
		setSize(getMaxLength() * this->zoom, getHeight());
		setZoom(zoom);
		repaint();
		sendChangeMessage();
	}

	ModifierKeys m = key.getModifiers();

	if (m.isCommandDown()) {

		Logger::getCurrentLogger()->writeToLog(String(key.getKeyCode()));

		if (key.getKeyCode() == 76) { // L
			for (int i = 0; i < tracks.size(); i++) {
				tracks.at(i)->toggleLoopSelection();
			}
		}
		else if (key.getKeyCode() == 67) { // C
			for (int i = 0; i < tracks.size(); i++) {
				tracks.at(i)->copySelectedRegions();
			}
		}
		else if (key.getKeyCode() == 68) { // D
			for (int i = 0; i < tracks.size(); i++) {
				tracks.at(i)->duplicateSelectedRegions();
			}
		}
		else if (key.getKeyCode() == 69) { // E
			
			openPianoRoll(dynamic_cast<MidiRegion*>(getCurrentTrack()->getCurrentRegion()));
		}
		else if (key.getKeyCode() == 77) { // M;
			openEventEditor(dynamic_cast<MidiRegion*>(getCurrentTrack()->getCurrentRegion()));
		}
		else if (key.getKeyCode() == 73) { // I
			for (int i = 0; i < tracks.size(); i++) {
				for (int j = 0; j < tracks.at(i)->getRegions().size(); j++) {
					Logger::getCurrentLogger()->writeToLog(String(tracks.at(i)->getRegions().at(j)->getSampleOffset()));
				}
			}
		}
		else if (key.getKeyCode() == 88) { // X
			for (int i = 0; i < tracks.size(); i++) {
				tracks.at(i)->removeSelectedRegions(true);
			}
		}
		else if (key.getKeyCode() == 84) { // T
			for (int i = 0; i < tracks.size(); i++) {
				tracks.at(i)->splitRegion();
			}
		}
	}

	return true;
}

void TrackNavigator::mouseUp(const MouseEvent& event) {
}

void TrackNavigator::mouseDrag(const MouseEvent& event) {
	if (event.mods.isLeftButtonDown() && event.mods.isAltDown()) {

		int start = UITools::snap(event.getMouseDownPosition().getX(), zoom / 4, 2);
		int end = UITools::snap(event.getOffsetFromDragStart().getX(), zoom / 4, 2);

		getSelector()->setSelectedRange(start, end);
	}
}

void TrackNavigator::mouseMove(const juce::MouseEvent& event) {

}

void TrackNavigator::mouseDown(const MouseEvent& event) {

	int x = event.x + scrollbarOffset;
	int y = event.y;

	if (isPlaying()) {
		allNotesOff();
	}

	if (!event.mods.isCtrlDown() && event.mods.isRightButtonDown()) {
		int width = getWidth();
		
		// float pos = (float)dragger->snap(x, zoom / 16) / width;
		double total = getMaxLength();
		double relative = total * x / width;
		setPosition(relative);
		marker->setSamplePosition(samplePosition);
		for (int i = 0; i < tracks.size(); i++) {
			tracks.at(i)->setSamplePosition(samplePosition);
			tracks.at(i)->setCurrentMarkerPosition(position);
		}
		sendChangeMessage();
	}
	//else
	if (Track* r = dynamic_cast<Track*>(event.eventComponent)) {
		for (int i = 0; i < tracks.size(); i++) {
			tracks.at(i)->setSelected(false);
			tracks.at(i)->clearSelection();
		}

		r->setSelected(true);
		currentTrack = r;
		deviceManager->setDefaultMidiOutput(currentTrack->getMidiOutputDevice());
		propertyView->selectionChanged(r);
		sendChangeMessage();
		repaint();
	}
	else {

		if (tracks.size() > 0) {
			if (event.mods.isLeftButtonDown()) {
				for (int i = 0; i < tracks.size(); i++) {

					Rectangle<int> bounds = tracks.at(i)->getBounds();
					bounds.setY(i * Project::DEFAULT_TRACK_HEIGHT);

					if (bounds.contains(x, y)) {
						currentTrack = tracks.at(i);
						Rectangle<int> selectorBounds = this->selector->getBounds();
						selectorBounds.setY(i * Project::DEFAULT_TRACK_HEIGHT);
						this->selector->setBounds(selectorBounds);
						this->selector->setSelectedRange(0, 0);
						tracks.at(i)->setSelected(true);
						propertyView->selectionChanged(tracks.at(i));

						for (int j = 0; j < tracks.at(i)->getRegions().size(); j++) {

							if (tracks.at(i)->getRegions().at(j)->getBounds().contains(x, y)) {
								tracks.at(i)->getRegions().at(j)->setSelected(true);
								propertyView->selectionChanged(tracks.at(i)->getRegions().at(j));
							}
							else {
								tracks.at(i)->getRegions().at(j)->setSelected(false);
							}
						}
						sendChangeMessage();
					}
					else {
						tracks.at(i)->setSelected(false);
						sendChangeMessage();
					}

				}
			}

		}
	}

}
