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
	// Timeline und alles Sample-bezogene beginnen bei kControlsWidth — links
	// davor liegt der Track-Control-Strip.
	this->timeLine->setBounds(kControlsWidth, 0, getWidth() - kControlsWidth, timelineOffset);
	this->timeLine->setSize(getWidth() - kControlsWidth, timelineOffset);
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
	this->dragger->setLockHorizontal(true);
	
	double beatsPerSecond = Project::getInstance()->getTempo() / 60.0;
	double samplesPerBeat = Project::getInstance()->getSampleRate() / beatsPerSecond;
	int raster = (int)(samplesPerBeat * zoom) / Project::getInstance()->getSampleRate(); // Viertelnoten
	dragger->setRaster(raster);

	Project::getInstance()->addChangeListener(this);
	addChangeListener(Mixer::getInstance());
	addAndMakeVisible(marker);
	marker->setAlwaysOnTop(true);
	marker->setBounds(kControlsWidth, 0, 2, getHeight());
	this->view = view;
	view->getHorizontalScrollBar().addListener(this);

	this->zoomDisplay = std::make_unique<ZoomDisplay>();
	addChildComponent(zoomDisplay.get());
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
	// Erst die Controls weg — wenn der Track unter ihnen gelöscht wird, hätten
	// sie einen Dangling-Pointer.
	trackControls.clear();
	for (std::vector<Track*>::iterator it = tracks.begin(); it != tracks.end(); ++it) {
		delete* it;
	}
	tracks.clear();
}

void TrackNavigator::moved() {
	// Beim horizontalen Scrollen verschiebt der Viewport den Navigator in
	// negative X-Koordinaten (getX() wird z.B. -100, wenn 100 px nach rechts
	// gescrollt wurde). Damit die TrackControl-Strips visuell am linken Rand
	// des sichtbaren Bereichs kleben bleiben, ziehen wir sie um den gleichen
	// Betrag nach rechts. Vertikal scrollen sie automatisch mit, weil sie
	// Children des Navigators sind und auf den Y-Koordinaten der Tracks liegen.
	const int viewX = juce::jmax(0, -getX());
	for (auto& tc : trackControls) {
		if (tc != nullptr)
			tc->setTopLeftPosition(viewX, tc->getY());
	}
	// Marker und Timeline bleiben Sample-bezogen — die scrollen mit.
}

void TrackNavigator::syncTrackControls() {
	// 1) Verwaiste Controls (Track wurde entfernt) löschen.
	trackControls.erase(
		std::remove_if(trackControls.begin(), trackControls.end(),
			[this](const std::unique_ptr<TrackControl>& tc) {
				if (tc == nullptr) return true;
				for (Track* t : tracks)
					if (t == tc->getTrack()) return false;
				return true;
			}),
		trackControls.end());

	// 2) Für jeden Track, der noch keinen Control hat, einen anlegen.
	for (Track* t : tracks) {
		if (t == nullptr) continue;
		bool found = false;
		for (auto& tc : trackControls)
			if (tc->getTrack() == t) { found = true; break; }
		if (!found) {
			auto control = std::make_unique<TrackControl>(t);
			addAndMakeVisible(control.get());
			trackControls.push_back(std::move(control));
		}
	}

	// 3) Alle Controls an die richtige Y-Position des zugehörigen Tracks
	//    schieben und auf dessen aktuelle Höhe bringen. Die X-Position folgt
	//    dem aktuellen Scroll-Offset, damit die Controls beim Sync nicht auf
	//    x=0 zurückspringen, während der Navigator nach rechts gescrollt ist.
	const int viewX = juce::jmax(0, -getX());
	for (auto& tc : trackControls) {
		if (tc == nullptr) continue;
		Track* t = tc->getTrack();
		if (t == nullptr) continue;
		tc->setBounds(viewX, t->getY(), kControlsWidth, t->getHeight());
		tc->syncFromTrack();
	}
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

	if (zoomDisplay != nullptr && view != nullptr) {
		// Center the overlay in the viewport's visible area. The TrackNavigator
		// scrolls inside `view`, so map the visible rect back into our own
		// coords via getViewArea().
		const int displayW = 320;
		const int displayH = 100;
		const auto visible = view->getViewArea();
		const int x = visible.getX() + (visible.getWidth()  - displayW) / 2;
		const int y = visible.getY() + (visible.getHeight() - displayH) / 2;
		zoomDisplay->setBounds(x, y, displayW, displayH);
		zoomDisplay->showZoom(zoom);
	}

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
			tracks.at(0)->setTopLeftPosition(kControlsWidth, timelineOffset);
		}

	}

	syncTrackControls();
	adjustHeight();
	sendChangeMessage();
}

void TrackNavigator::adjustHeight()
{
	int height = 0;

	for (int i = 0; i < tracks.size(); i++) {
		height += tracks.at(i)->getHeight();
	}

	this->marker->setBounds(kControlsWidth, 0, 2, getHeight());

	setSize(getWidth(), height + timelineOffset);
}

// Bestehende isInterestedInDragSource erweitern:
bool TrackNavigator::isInterestedInDragSource(const SourceDetails& dragSourceDetails)
{
	juce::String name = dragSourceDetails.description.toString();
	if (name.endsWith(".wav") || name.endsWith(".mp3") ||
		name.endsWith(".flac") || name.endsWith(".ogg") ||
		name.endsWith(".aiff") || name.endsWith(".aif")) {
		return true;
	}
	return false;	}

// Neue Methode f�r externe Datei-Drops hinzuf�gen:
bool TrackNavigator::isInterestedInFileDrag(const StringArray& files)
{
	// Pr�fe, ob mindestens eine Audio-Datei dabei ist
	for (const auto& filename : files)
	{
		juce::File file(filename);
		juce::String extension = file.getFileExtension().toLowerCase();

		if (extension == ".wav" || extension == ".mp3" ||
			extension == ".flac" || extension == ".ogg" ||
			extension == ".aiff" || extension == ".aif" ||
			extension == ".m4a" || extension == ".wma")
		{
			return true;
		}
	}
	return false;
}

void TrackNavigator::filesDropped(const StringArray& files, int x, int y)
{
	// Verarbeite alle Audio-Dateien
	for (const auto& filename : files)
	{
		juce::File file(filename);
		juce::String extension = file.getFileExtension().toLowerCase();

		// Pr�fe, ob es eine unterst�tzte Audio-Datei ist
		if (extension == ".wav" || extension == ".mp3" ||
			extension == ".flac" || extension == ".ogg" ||
			extension == ".aiff" || extension == ".aif" ||
			extension == ".m4a" || extension == ".wma")
		{
			for (int i = 0; i < tracks.size(); i++) {
				if (tracks.at(i)->isSelected()) {					
					tracks.at(i)->addRegion(file.getFileNameWithoutExtension(), file, Project::getInstance()->getSampleRate());
					Project::getInstance()->addAudioFile(file.getFileNameWithoutExtension(), file.getFullPathName());
					repaint();
				}
			}

		}
	}

	// Layout aktualisieren
	adjustHeight();
	repaint();
	sendChangeMessage();
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
	marker->setSamplePosition(this->samplePosition);
	transportDisplay->setTransportState(samplePosition, Project::getInstance()->getSampleRate(), Project::getInstance()->getTempo(), 4, 4);
	if (pianoRoll != nullptr)
		pianoRoll->setPlaybackPosition(this->samplePosition);
	repaint();
}

ValueTree TrackNavigator::getConfiguration()
{
	ValueTree v("Tracks");

	for (int i = 0; i < tracks.size(); i++) {

		ValueTree track("Track");
		track.setProperty("name", tracks.at(i)->getName(), nullptr);

		// Setze den Typ unabh�ngig von den Regionen
		if (tracks.at(i)->getType() == Track::AUDIO) {
			track.setProperty("type", "audio", nullptr);
		}
		else if (tracks.at(i)->getType() == Track::INSTRUMENT) {
			track.setProperty("type", "instrument", nullptr);
			track.setProperty("midiChannel", tracks.at(i)->getMidiChannel(), nullptr);
			track.setProperty("plugin", tracks.at(i)->getInstrumentName(), nullptr);
		}
		else {
			track.setProperty("type", "midi", nullptr);
			track.setProperty("midiChannel", tracks.at(i)->getMidiChannel(), nullptr);
			track.setProperty("interface", tracks.at(i)->getMidiOutputDevice(), nullptr);
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

					DBG("Saving " + String(mr->getNumMessages()) + " MIDI messages");

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
							e.setProperty("timestamp", (int64)msg->getTimeStamp(), nullptr);

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
	this->samplePosition = alignToBufferSize(this->samplePosition);

	marker->setSamplePosition(samplePosition);
    
	transportDisplay->setTransportState(samplePosition, Project::getInstance()->getSampleRate(), Project::getInstance()->getTempo(), 4, 4);
	startTimer(50);
}

int TrackNavigator::getSamplePosition() {
	return samplePosition;
}

void TrackNavigator::ResetMarkerPosition()
{
    setPosition(0);
    const bool transportActive = isPlaying() || isRecording();
    for (int i = 0; i < tracks.size(); i++) {
        tracks.at(i)->setSamplePosition(0, transportActive);
        tracks.at(i)->setCurrentMarkerPosition(0);
    }
    
    // Synchronisiere auch die PluginModules, sonst sieht ein
    // tempo-sync-empfindlicher Synth (Avenger) beim nächsten Play
    // einen Transport-Sprung und spinnt.
    for (int i = 0; i < Project::getInstance()->getRoot()->getModules()->size(); i++) {
        if (auto* module = dynamic_cast<PluginModule*>(
                Project::getInstance()->getRoot()->getModules()->at(i))) {
            module->requestPositionReset(0);
            module->requestReset();
        }
    }
    
    sendChangeMessage();
}

void TrackNavigator::setSamplePosition(int position) {

	this->samplePosition = alignToBufferSize(position);

	if (loopEnabled) {
		if (this->samplePosition > loopEnd * Project::getInstance()->getSampleRate()) {
			for (int i = 0; i < tracks.size(); i++)
				tracks.at(i)->sendAllNotesOff();
			this->samplePosition = loopStart * Project::getInstance()->getSampleRate();
		}
	}

	for (int i = 0; i < tracks.size(); i++) {
		tracks.at(i)->setSamplePosition(this->getSamplePosition());
	}
    
    for (int i = 0; i < Project::getInstance()->getRoot()->getModules()->size(); i++) {
        if (auto* module = dynamic_cast<PluginModule*>(
                Project::getInstance()->getRoot()->getModules()->at(i))) {
            module->setCurrentSample(this->samplePosition);
        }
    }

    
	this->position = (float)samplePosition / Project::getInstance()->getSampleRate();
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
    if (playing)
        Logger::getCurrentLogger()->writeToLog("Playing start");
    else
        Logger::getCurrentLogger()->writeToLog("Playing stop");

	this->playing = playing;
    if (playing) {
        startTimer(50);
    }
	else {
		stopTimer();
		allNotesOff();
	}
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
			module->getPlayHead()->setRecording(false);
            module->getPlayHead()->setPlaying(playing);
            if (playing) {
                module->requestPositionReset(this->samplePosition);
                module->requestReset();
            }
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
    if (recording)
        Logger::getCurrentLogger()->writeToLog("Recording start");
    else
        Logger::getCurrentLogger()->writeToLog("Recording stop");
    
	if (Project::getInstance()->getRoot() != nullptr) {
		for (int i = 0; i < Project::getInstance()->getRoot()->getModules()->size(); i++) {

			PluginModule* module = nullptr;

			// make sure we do not send this event to a plugin module

			if ((module = dynamic_cast<PluginModule*>(Project::getInstance()->getRoot()->getModules()->at(i))) != nullptr) {
                
                if (recording) {
                    module->getPlayHead()->setRecording(true);
                    module->getPlayHead()->setPlaying(true);
                }
                else {
                    module->getPlayHead()->setRecording(false);
                    module->getPlayHead()->setPlaying(false);
                    module->requestReset();
                }
			}
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

	pianoRoll->onPlayToggle = [this]() {
		setPlaying(!isPlaying());
	};

	pianoRoll->onMarkerPositionChanged = [this](double samplePos) {
		double posInSeconds = samplePos / Project::getInstance()->getSampleRate();
		setPosition(posInSeconds);
		for (int i = 0; i < tracks.size(); i++) {
			tracks.at(i)->setSamplePosition(this->samplePosition);
			tracks.at(i)->setCurrentMarkerPosition(this->position);
		}
		sendChangeMessage();
	};

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

	setBounds(getX(), getY(), kControlsWidth + (int)(getMaxLength() * this->zoom), height);

	this->marker->setSize(2, height);
	this->marker->setSampleLength(getMaxLength() * 300);
	this->selector->setSize(getWidth(), getHeight());
	this->timeLine->setBounds(kControlsWidth, 0, getWidth() - kControlsWidth, timelineOffset);
	this->timeLine->setSize(getWidth() - kControlsWidth, timelineOffset);

	repaint();
	sendChangeMessage();

	return track;
}

void TrackNavigator::addTrack(Track* track) {

	track->setNavigator(this);

	// track->setInterceptsMouseClicks(false, true);

	track->addMouseListener(this, true);

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
	track->setBounds(kControlsWidth, yPos, p->getTrackLength() * this->zoom, track->getHeight());

	this->tracks.push_back(track);
	this->currentTrack = track;

	addAndMakeVisible(track);
	syncTrackControls();

	track->addChangeListener(this);
	track->setZoom(this->zoom);
	track->setBPM(Project::getInstance()->getTempo());

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

	setBounds(getX(), getY(), kControlsWidth + (int)(getMaxLength() * this->zoom), height);

	this->marker->setSize(2, getHeight());
	this->marker->setSampleLength(getMaxLength() * 300);
	this->selector->setSize(getWidth(), getHeight());
	this->timeLine->setBounds(kControlsWidth, 0, getWidth() - kControlsWidth, timelineOffset);
	this->timeLine->setSize(getWidth() - kControlsWidth, timelineOffset);

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

	syncTrackControls();
	adjustHeight();
	repaint();
	sendChangeMessage();

}

Track* TrackNavigator::detachSelectedTrack(int& outIndex)
{
	for (int i = 0; i < (int)tracks.size(); i++) {
		if (tracks[i]->isSelected()) {
			Track* track = tracks[i];
			outIndex = i;
			tracks.erase(tracks.begin() + i);
			track->removeChangeListener(this);
			removeChildComponent(track);

			// Restack remaining tracks
			if (!tracks.empty()) {
				tracks[0]->setTopLeftPosition(tracks[0]->getX(), timelineOffset);
				for (int j = 1; j < (int)tracks.size(); j++)
					tracks[j]->setTopLeftPosition(tracks[j-1]->getX(),
					                              tracks[j-1]->getY() + tracks[j-1]->getHeight());
			}

			if (currentTrack == track)
				currentTrack = tracks.empty() ? nullptr : tracks[0];

			syncTrackControls();
			adjustHeight();
			repaint();
			sendChangeMessage();
			return track;
		}
	}
	return nullptr;
}

void TrackNavigator::insertTrackAt(Track* track, int index)
{
	index = jlimit(0, (int)tracks.size(), index);
	tracks.insert(tracks.begin() + index, track);
	track->setNavigator(this);
	track->addChangeListener(this);
	addAndMakeVisible(track);

	// Restack all tracks
	int yPos = timelineOffset;
	for (auto* t : tracks) {
		t->setTopLeftPosition(kControlsWidth, yPos);
		yPos += t->getHeight();
	}

	currentTrack = track;
	syncTrackControls();
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
	this->marker->setDrawingBounds(kControlsWidth, 25, getWidth() - kControlsWidth, getHeight());
	this->timeLine->setBounds(kControlsWidth, 0, getWidth() - kControlsWidth, timelineOffset);

	syncTrackControls();
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

	if (!hasFocus) {
		return false;
	}

	if (key == KeyPress::spaceKey) {
		setPlaying(!isPlaying());
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
			// Replace clipboard contents — otherwise repeated Cmd-C pushes
			// duplicates of the same region and Cmd-V pastes them all.
			Session::getInstance()->clearRegionsClipboard();
			for (int i = 0; i < tracks.size(); i++) {
				tracks.at(i)->copySelectedRegions();
			}
		}
		else if (key.getKeyCode() == 86) { // V — paste at playhead into current track
			Track* dest = getCurrentTrack();
			if (dest != nullptr)
				dest->pasteRegions((long)samplePosition);
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

	return false;
}

int TrackNavigator::alignToBufferSize(int samplePosition) {
	int bufferSize = Project::getInstance()->getBufferSize();
	return (samplePosition / bufferSize) * bufferSize;
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

	// event.x ist relativ zur Komponente, die geklickt wurde
	// Wir brauchen die Position relativ zum Navigator
	int x = event.getEventRelativeTo(this).x;
	int y = event.getEventRelativeTo(this).y;

	if (isPlaying()) {
		allNotesOff();
	}

	if (!event.mods.isCtrlDown() && event.mods.isRightButtonDown()) {
		// x ist relativ zum Navigator (inkl. gescrolltem Bereich). Sample 0
		// liegt aber bei x = kControlsWidth, weil davor der TrackControl-Strip
		// sitzt — daher den Offset abziehen, sonst springt der Playhead beim
		// Rechtsklick auf den Track-Anfang in 3.5 s rein.
		double positionInSeconds = (double)(x - kControlsWidth) / zoom;
		if (positionInSeconds < 0.0) positionInSeconds = 0.0;
		setPosition(positionInSeconds);
		marker->setSamplePosition(getSamplePosition());
		// While stopped, manually moving the playhead must not fire notes
		// at the new position. Pass dispatchEvents=false so the track
		// updates its position silently (samplePosition + lastActiveRegion
		// still move so getCurrentRegion/PianoRoll work).
		const bool transportActive = isPlaying() || isRecording();
		for (int i = 0; i < tracks.size(); i++) {
			tracks.at(i)->setSamplePosition(samplePosition, transportActive);
			tracks.at(i)->setCurrentMarkerPosition(position);
		}
		sendChangeMessage();
		marker->repaint();
        for (int i = 0; i < Project::getInstance()->getRoot()->getModules()->size(); i++)
        {
            if (auto* module = dynamic_cast<PluginModule*>(Project::getInstance()->getRoot()->getModules()->at(i)))
            {
                // module->requestReset();
                module->requestPositionReset(samplePosition);
            }
        }
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
