/*
  ==============================================================================

	WaveNavigator.cpp
	Created: 24 Nov 2016 8:04:54am
	Author:  Matthias Pueski

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "TrackNavigator.h"
#include "../TrackUtil.h"
#include "../SynthEditor.h"
#include "AudioRegion.h"
#include "MidiRegion.h"
#include "../Project/Project.h"
#include "../Session.h"
#include "../Mixer.h"
#include <iterator>
#include <iostream>
#include "../ExtendedFileBrowser.h"
#include "../PianoRollComponent.h"
#include "../MainComponent.h"
#include "../MidiRegionEventEditor.h"
#include "../PluginModule.h"
#include "../MidiProcessor.h"
#include "../TrackIn.h"
#include "../AudioManager.h"
#include "../MixerPanel.h"
#include "MasterChannelPanel.h"
#include "../Settings.h"

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
	// Vertikal fixierte Timeline: sie muss über den Tracks/Regionen liegen,
	// die beim vertikalen Scrollen unter ihr durchlaufen. Der Marker und die
	// TrackControls werden danach angelegt und bleiben dadurch über ihr.
	timeLine->setAlwaysOnTop(true);
	this->deviceManager = deviceManager;
	this->dragFormatManager.registerBasicFormats();
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
	// Der Marker dient nur noch als Positions-Speicher (getSamplePosition); gezeichnet
	// wird er ausschliesslich in TrackNavigator::paint() ueber navigatorXForSample(),
	// damit er EXAKT die gleiche Sample->X-Abbildung wie Tracks/Timeline nutzt.
	// Frueher zeichnete sich das Child zusaetzlich selbst (alwaysOnTop) ueber sein
	// eigenes, im Konstruktor eingefrorenes sampleRate — wich die Geraete-Sample-Rate
	// davon ab, sass die Linie daneben. Als unsichtbares Child vermeiden wir das.
	addChildComponent(marker);
	this->view = view;
	view->getHorizontalScrollBar().addListener(this);

	this->zoomDisplay = std::make_unique<ZoomDisplay>();
	addChildComponent(zoomDisplay.get());
	setWantsKeyboardFocus(true);
	setMouseClickGrabsKeyboardFocus(true);
	setInterceptsMouseClicks(true, true);

	// Middle-Mouse-Pan: Forwarder bekommt Mausevents von uns selbst und allen
	// Kindkomponenten, damit Pannen auch ueber Tracks/Regions/Timeline geht.
	panForwarder = std::make_unique<PanMouseForwarder>(*this);
	addMouseListener(panForwarder.get(), true);

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
	if (panForwarder != nullptr)
		removeMouseListener(panForwarder.get());

	// Component-Listener vom Viewport abmelden bevor wir sterben.
	if (listenedViewport != nullptr) {
		listenedViewport->removeComponentListener(this);
		listenedViewport = nullptr;
	}

	// Listener-Registrierungen aus dem Konstruktor sauber zurueckziehen — sonst
	// schickt Project::setTempo() o.ae. ChangeMessages an einen toten this-
	// Pointer (Crash in ChangeBroadcaster::callListeners beim naechsten Open).
	if (auto* p = Project::getInstance())
		p->removeChangeListener(this);
	if (auto* m = Mixer::getInstance())
		removeChangeListener(m);

	if (view != nullptr) {
		view->getHorizontalScrollBar().removeListener(this);
	}

	for (auto* t : tracks) {
		if (t != nullptr) t->removeChangeListener(this);
	}

	timeLine = nullptr;

	for (std::vector<Track*>::iterator it = tracks.begin(); it != tracks.end(); ++it) {
		delete* it;
	}

	delete dragger;
	delete marker;
}

void TrackNavigator::paint(Graphics& g)
{
	// Der Positions-Marker wird in paintOverChildren() gezeichnet, damit er UEBER
	// den Tracks/Regionen liegt (paint() laeuft vor den Child-Components, dort
	// wuerde die Linie hinter den Regions verschwinden).
}

void TrackNavigator::clearTracks() {
	// Audio thread holds raw Track* pointers via TrackIn modules (set in
	// AddTrackAction::buildAudio/Midi/Instrument). If we delete the Track
	// objects below while the callback is mid-TrackIn::process(), it reads
	// freed memory in Track::isRecording() — exactly the ASan UAF seen
	// after `project.clear()` from JS. Two-step fix:
	//   1) Hold the writer lock so the audio thread's ScopedTryLock in
	//      MainComponent::getNextAudioBlock fails for one buffer (silence).
	//   2) Before the deletes, null out every TrackIn's Track* so that any
	//      callback AFTER we release the lock sees track == nullptr and
	//      bails out cleanly (TrackIn::process already null-guards on
	//      `track`). Without (2) the UAF re-fires on the next buffer even
	//      with the lock — clearTracks-from-JS leaves the TrackIn modules
	//      in the editor graph by design (root modules survive).
	const juce::ScopedLock graphLock(
		AudioManager::getInstance()->getProcessGraphLock());

	if (editor != nullptr) {
		if (auto* root = editor->getModule()) {
			if (auto* mods = root->getModules()) {
				for (auto* m : *mods) {
					if (auto* ti = dynamic_cast<TrackIn*>(m))
						ti->setTrack(nullptr);
				}
			}
		}

		// UI side: mixer channel strips hold a raw Track* via setTrack() and
		// paint() reads track->getType() — the strips OUTLIVE clearTracks
		// because they belong to MixerPanel, not the navigator. Without this
		// null-out, the next AppKit repaint (triggered by any state change
		// after project.clear, even the mixer's own change broadcast) dereffs
		// a freed Track in MasterChannelPanel::paint. Mirror the TrackIn
		// detach for every channel strip whose track is about to die.
		if (auto mixer = editor->getMixer()) {
			for (auto* panel : mixer->getChannels()) {
				if (panel == nullptr) continue;
				Track* t = panel->getTrack();
				if (t == nullptr) continue;
				// Only null strips whose track is in the about-to-be-deleted
				// set; non-track strips (master / sub-mix) survive untouched.
				if (std::find(tracks.begin(), tracks.end(), t) != tracks.end())
					panel->setTrack(nullptr);
			}
		}
	}

	// Erst die Controls weg — wenn der Track unter ihnen gelöscht wird, hätten
	// sie einen Dangling-Pointer.
	trackControls.clear();

	// PianoRoll laeuft als 30-Hz-Timer und haelt einen MidiRegion*-Pointer.
	// JUCE meldet uns das componentBeingDeleted, wenn die Region verschwindet,
	// aber der PianoRollEditor selbst bleibt sonst nach dem File-Wechsel mit
	// gestorbener Region und totem Track-Backref leben — sicherer ist, ihn
	// jetzt mit den Tracks gemeinsam wegzuwerfen. Owner ist seit dem Docking-
	// Refactor das pianoroll-DockablePanel; closePianoRollPanel zerstoert
	// Panel + Editor in einem Rutsch, die SafePointer nullt sich selbst.
	if (pianoRoll != nullptr) {
		if (auto* mc = findParentComponentOfClass<MainComponent>())
			mc->closePianoRollPanel();
		pianoRoll = nullptr;
	}

	// Event-Editor analog mit abreissen — der haengt an einer MidiRegion,
	// die gleich zerstoert wird. Ohne dieses Close haette das Dock-Panel
	// nach dem File-Wechsel einen dangling MidiRegion* in seinen
	// ListBoxModel-Callbacks. closeEventEditorPanel() ist no-op wenn
	// nichts offen ist, also unbedingt billig.
	if (auto* mc = findParentComponentOfClass<MainComponent>())
		mc->closeEventEditorPanel();

	currentTrack = nullptr;

	for (std::vector<Track*>::iterator it = tracks.begin(); it != tracks.end(); ++it) {
		// Mixer haelt eine parallele tracks-Liste (Mixer::addTrack) UND einen
		// Listener-Eintrag pro Track. Beides austragen, sonst bleibt nach
		// einem File-Wechsel ein dangling Track* im Mixer-Vector und/oder ein
		// dangling Listener-Pointer — der naechste Mixer::sendChangeMessage()
		// kracht dann in ChangeBroadcaster::callListeners.
		if (auto* m = Mixer::getInstance())
			m->removeTrack(*it);
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
	// Marker bleibt Sample-bezogen — er scrollt horizontal mit.
	// Die Timeline scrollt horizontal ebenfalls mit (Sample-bezogen), muss
	// aber vertikal am oberen Rand des sichtbaren Bereichs fixiert bleiben,
	// sonst läuft sie beim vertikalen Scrollen mit nach oben weg. Beim
	// vertikalen Scrollen wird der Navigator in negative Y-Koordinaten
	// verschoben (getY() < 0) — um den gleichen Betrag ziehen wir die
	// Timeline wieder nach unten.
	const int viewY = juce::jmax(0, -getY());
	if (timeLine != nullptr)
		timeLine->setTopLeftPosition(timeLine->getX(), viewY);
}

void TrackNavigator::applyAutoArm(Track* selected) {
	if (selected == nullptr)
		return;
	if (!Settings::getInstance()->getAutoTrackArming())
		return;

	for (Track* t : tracks) {
		if (t == nullptr) continue;
		t->setRecording(t == selected);
	}

	// Die "R"-Toggles in den TrackControl-Strips an den neuen Arm-Zustand
	// angleichen.
	syncTrackControls();
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
			// Über die alwaysOnTop-Timeline heben, damit der linke Steuer-Strip
			// an der oberen linken Ecke nicht von der Timeline überdeckt wird.
			control->setAlwaysOnTop(true);
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
	// getRoot() ist nullptr, solange kein Modular-Baum gesetzt ist (z.B. Projekt
	// nur mit Tracks) — dann darf ->getModules() nicht dereferenziert werden.
	auto* root = Project::getInstance()->getRoot();
	if (root == nullptr) return;
	auto* mods = root->getModules();
	if (mods == nullptr) return;

	for (int i = 0; i < (int) mods->size(); i++) {

		for (int j = 0; j < 16; j++) {
			Project::getInstance()->getMidiProcessor()->sendMidiStopMessage(mods->at(i), j);

			for (int k = 0; k < 128; k++) {
				Project::getInstance()->getMidiProcessor()->sendGateMessage(mods->at(i), j, 0, k, false);
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
	if (timeLine != nullptr) {
		timeLine->setLength(getMaxLength());
		timeLine->setZoom(zoom);
	}
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

void TrackNavigator::relayoutTracks()
{
	// Y-Positionen alle Tracks aus aktuellen Hoehen neu stapeln. Selbe
	// Logik wie nach insertTrack / Zoom-Resize, nur als Helper extrahiert,
	// damit Track::mouseDrag waehrend des Alt-Drag-Resizes live aufrufen kann
	// — ohne diesen Aufruf bleibt der gerade geziehte Track an seiner
	// urspruenglichen Y haengen, waehrend die gespiegelten Geschwister
	// schon mit der neuen Hoehe gezeichnet werden, was visuell wie ein
	// Ueberlappen aussieht.
	int yPos = timelineOffset;
	for (auto* t : tracks) {
		if (t == nullptr) continue;
		t->setTopLeftPosition(kControlsWidth, yPos);
		yPos += t->getHeight();
	}
	syncTrackControls();
	adjustHeight();
	repaint();
}

void TrackNavigator::adjustHeight()
{
	int height = timelineOffset;

	for (int i = 0; i < tracks.size(); i++) {
		height += tracks.at(i)->getHeight();
	}

	// Mindestens so groß wie der sichtbare Bereich, damit Drops auf "leeren"
	// Bereich (kein Track) trotzdem den Navigator erreichen. Vorher wurde
	// getParentComponent() benutzt — das ist aber bei eingebetteten Viewports
	// der interne Viewport::ContentHolder, dessen Hoehe wiederum vom Navigator
	// selbst abhaengt. Wir suchen also den naechsten echten Viewport in der
	// Hierarchie und nehmen dessen Sichtflaeche.
	int minHeight = 200;
	int minWidth  = 600;   // Fallback-Breite — sonst bleibt der Navigator beim
	                       // ersten Aufruf (vor dem Layout des Viewports) auf
	                       // getWidth()=0 und wird gar nicht erst gerendert.
	if (auto* vp = findParentComponentOfClass<juce::Viewport>()) {
		const int vh = vp->getViewArea().getHeight();
		const int vw = vp->getViewArea().getWidth();
		if (vh > 0) minHeight = vh;
		if (vw > 0) minWidth  = vw;
	}
	if (minHeight > height) height = minHeight;

	// Content-Breite zentral hier ableiten, nicht nur im Zoom-Handler. Sonst
	// bleibt der Navigator nach Track-/Clip-Erzeugung auf der alten (zu
	// schmalen) Breite stehen, bis ein Zoom-Event setSize() mit
	// getMaxLength()*zoom triggert — genau der gemeldete "erst nach Einzoomen
	// richtig"-Effekt. kControlsWidth, weil Timeline/Marker/Tracks dort starten.
	const int contentWidth = kControlsWidth + (int)(getMaxLength() * zoom);
	int width = juce::jmax(contentWidth, minWidth);

	setSize(width, height);
	this->marker->setSize(2, getHeight());
	repaint();
}

void TrackNavigator::parentSizeChanged()
{
	adjustHeight();
}

void TrackNavigator::parentHierarchyChanged()
{
	// Viewport-Listener neu verkabeln, falls wir gerade in einen anderen
	// Viewport gepackt wurden (oder erstmals einen kriegen). Unbeobachteter
	// Resize wuerde adjustHeight nie triggern und der leere Navigator bliebe
	// auf timelineOffset hoch -> kein Platz fuer Drag&Drop ohne Tracks.
	juce::Viewport* vp = findParentComponentOfClass<juce::Viewport>();
	if (vp != listenedViewport) {
		if (listenedViewport != nullptr)
			listenedViewport->removeComponentListener(this);
		listenedViewport = vp;
		if (listenedViewport != nullptr)
			listenedViewport->addComponentListener(this);
	}
	adjustHeight();
}

void TrackNavigator::componentMovedOrResized(juce::Component& which, bool /*wasMoved*/, bool wasResized)
{
	if (!wasResized) return;
	if (&which != listenedViewport) return;
	adjustHeight();
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

Track* TrackNavigator::createAudioTrackForDrop()
{
	if (editor != nullptr && editor->getModule() != nullptr) {
		return TrackUtil::createAudioTrack(editor, this, editor->getModule());
	}

	// Fallback: ohne Editor/Module kennen wir keinen Modul-Graph — dann
	// wenigstens einen reinen Track ohne Verdrahtung anlegen, damit die
	// Region irgendwo landet.
	return addTrack(Track::Type::AUDIO, Project::getInstance()->getSampleRate());
}

Track* TrackNavigator::trackAtY(int y) const
{
	int yPos = timelineOffset;
	for (auto* t : tracks) {
		const int h = t->getHeight();
		if (y >= yPos && y < yPos + h)
			return t;
		yPos += h;
	}
	return nullptr;
}

// Liefert den Pfad der ersten gehaltenen Audiodatei (oder leer).
static juce::String firstAudioFilePath(const juce::StringArray& files)
{
	for (const auto& filename : files)
	{
		juce::File file(filename);
		juce::String ext = file.getFileExtension().toLowerCase();
		if (ext == ".wav" || ext == ".mp3" || ext == ".flac" || ext == ".ogg" ||
			ext == ".aiff" || ext == ".aif" || ext == ".m4a" || ext == ".wma")
			return filename;
	}
	return {};
}

void TrackNavigator::filesDropped(const StringArray& files, int x, int y)
{
	clearDragPreview();

	// Track an der Drop-Y-Position bestimmen — gibt es dort keinen, legen wir
	// genau einen neuen Audio-Track an, in den alle gedroppten Dateien
	// fallen (statt für jede Datei einen separaten Track zu erzeugen).
	Track* target = trackAtY(y);
	const double sr = Project::getInstance()->getSampleRate();

	// Drop-X -> grid-gesnappte Startposition. Mehrere Dateien werden ab dieser
	// Position hintereinander gelegt (Cursor wird je Region weitergeschoben),
	// damit sie nicht uebereinander landen.
	long cursor = -1;

	for (const auto& filename : files)
	{
		juce::File file(filename);
		juce::String extension = file.getFileExtension().toLowerCase();

		if (extension == ".wav" || extension == ".mp3" ||
			extension == ".flac" || extension == ".ogg" ||
			extension == ".aiff" || extension == ".aif" ||
			extension == ".m4a" || extension == ".wma")
		{
			if (target == nullptr) {
				target = createAudioTrackForDrop();
			}

			if (target != nullptr) {
				if (cursor < 0)
					cursor = snapSampleToGrid(target, sampleForNavigatorX(x));

				target->addRegion(file.getFileNameWithoutExtension(), file, sr, cursor, 0);
				Project::getInstance()->addAudioFile(file.getFileNameWithoutExtension(), file.getFullPathName());

				long lenSamples = (long)(audioFileLengthSeconds(file) * sr);
				cursor += juce::jmax((long)1, lenSamples);
			}
		}
	}

	if (target != nullptr) {
		for (auto* t : tracks)
			t->setSelected(t == target);
	}

	// Layout aktualisieren
	adjustHeight();
	repaint();
	sendChangeMessage();
}

void TrackNavigator::itemDropped(const SourceDetails& dragSourceDetails)
{
	clearDragPreview();

	juce::File file = juce::File(dragSourceDetails.description.toString());

	Track* target = trackAtY(dragSourceDetails.localPosition.getY());
	if (target == nullptr) {
		target = createAudioTrackForDrop();
	}

	if (target == nullptr) {
		Logger::getCurrentLogger()->writeToLog("Drop: konnte keinen Audio-Track anlegen.");
		return;
	}

	const double sr = Project::getInstance()->getSampleRate();
	const long samplePos = snapSampleToGrid(target, sampleForNavigatorX(dragSourceDetails.localPosition.getX()));

	target->addRegion(file.getFileNameWithoutExtension(), file, sr, samplePos, 0);
	Project::getInstance()->addAudioFile(file.getFileNameWithoutExtension(), file.getFullPathName());

	for (auto* t : tracks)
		t->setSelected(t == target);

	adjustHeight();
	repaint();
	sendChangeMessage();

	Logger::getCurrentLogger()->writeToLog(dragSourceDetails.description.toString());
}

// === Drag-Preview ==========================================================

void TrackNavigator::fileDragEnter(const StringArray& files, int x, int y) { fileDragMove(files, x, y); }

void TrackNavigator::fileDragMove(const StringArray& files, int x, int y)
{
	const juce::String path = firstAudioFilePath(files);
	if (path.isEmpty()) { clearDragPreview(); return; }
	updateDragPreview(path, x, y);
}

void TrackNavigator::fileDragExit(const StringArray& /*files*/) { clearDragPreview(); }

void TrackNavigator::itemDragEnter(const SourceDetails& d) { itemDragMove(d); }

void TrackNavigator::itemDragMove(const SourceDetails& d)
{
	updateDragPreview(d.description.toString(), d.localPosition.getX(), d.localPosition.getY());
}

void TrackNavigator::itemDragExit(const SourceDetails& /*d*/) { clearDragPreview(); }

void TrackNavigator::updateDragPreview(const juce::String& filePath, int x, int y)
{
	juce::File file(filePath);
	Track* target = trackAtY(y);

	const long snapped = snapSampleToGrid(target, sampleForNavigatorX(x));
	const double seconds = audioFileLengthSeconds(file);
	const int w = juce::jmax(2, juce::roundToInt(seconds * (double)zoom));
	const int px = navigatorXForSample(snapped);

	int ry, rh;
	if (target != nullptr) {
		ry = target->getY();
		rh = target->getHeight();
	}
	else if (!tracks.empty()) {
		Track* last = tracks.back();
		ry = last->getY() + last->getHeight();
		rh = last->getHeight();
	}
	else {
		ry = timelineOffset;
		rh = 80;
	}

	dragPreviewSample = snapped;
	dragPreviewRect = juce::Rectangle<int>(px, ry, w, rh);
	dragPreviewActive = true;
	repaint();
}

void TrackNavigator::clearDragPreview()
{
	if (!dragPreviewActive) return;
	dragPreviewActive = false;
	repaint();
}

long TrackNavigator::sampleForNavigatorX(int x) const
{
	if (zoom <= 0.0f) return 0;
	const double sr = Project::getInstance()->getSampleRate();
	double localPx = (double)x - (double)kControlsWidth;
	if (localPx < 0.0) localPx = 0.0;
	return (long)(localPx * sr / (double)zoom);
}

int TrackNavigator::navigatorXForSample(long sample) const
{
	const double sr = Project::getInstance()->getSampleRate();
	if (sr <= 0.0) return kControlsWidth;
	return kControlsWidth + juce::roundToInt((double)sample * (double)zoom / sr);
}

long TrackNavigator::gridSamplesForTrack(Track* target) const
{
	if (target != nullptr) {
		if (!target->isGridEnabled()) return 0;
		return target->getGridSizeInSamples();
	}
	// Default: Viertelnote bei Projekt-Tempo (so starten neue Drop-Tracks).
	const double sr = Project::getInstance()->getSampleRate();
	const double bpm = Project::getInstance()->getTempo();
	if (bpm <= 0.0) return 0;
	return (long)(sr / (bpm / 60.0));
}

long TrackNavigator::snapSampleToGrid(Track* target, long sample) const
{
	const long grid = gridSamplesForTrack(target);
	if (grid <= 0) return juce::jmax((long)0, sample);
	const long snapped = ((sample + grid / 2) / grid) * grid;
	return juce::jmax((long)0, snapped);
}

double TrackNavigator::audioFileLengthSeconds(const juce::File& file)
{
	if (file.getFullPathName() == dragPreviewCachedPath)
		return dragPreviewCachedSeconds;

	double seconds = 0.0;
	std::unique_ptr<juce::AudioFormatReader> reader(dragFormatManager.createReaderFor(file));
	if (reader != nullptr && reader->sampleRate > 0.0)
		seconds = (double)reader->lengthInSamples / reader->sampleRate;

	dragPreviewCachedPath = file.getFullPathName();
	dragPreviewCachedSeconds = seconds;
	return seconds;
}

void TrackNavigator::timerCallback()
{
	marker->setSamplePosition(this->samplePosition);
	transportDisplay->setTransportState(samplePosition, Project::getInstance()->getSampleRate(), Project::getInstance()->getTempo(), 4, 4);
	if (pianoRoll != nullptr)
		pianoRoll->getRoll().setPlaybackPosition(this->samplePosition);

	// Pending Automation-Werte aus dem Audio-Thread auf die Plugin-Parameter
	// anwenden — auf dem Message-Thread, damit Plugin-Editoren (z.B.
	// MainUI::parameterChanged) nicht synchron aus dem Audio-Callback heraus
	// getriggert werden und crashen.
	for (auto* t : tracks)
		if (t != nullptr) t->applyPendingAutomation();

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
					// Trim-Laenge bevorzugt aus dem sample-akkuraten Member
					// trimmedNumSamples (vom Konstruktor / Resize-Handler /
					// Consolidate gepflegt). Fallback auf die Pixel-Math fuer
					// alte Code-Pfade, die das Feld noch nicht setzen — die
					// Rueckrechnung Pixel->Samples verliert pro Round-Trip bis
					// zu eine halbe Pixelbreite, bei niedrigem Zoom mehrere ms.
					const double sr = Project::getInstance()->getSampleRate();
					const float zoom = ar->getZoom();
					long visibleSamples = ar->getTrimmedNumSamples();
					if (visibleSamples <= 0) {
						visibleSamples = (zoom > 0.0f && sr > 0.0)
							? (long) std::round((double) ar->getWidth() / zoom * sr)
							: (long) ar->getNumSamples();
					}

					// Regions without a clipRefId or with no audio content cannot
					// be reconstructed on load — SmartSample::loadFromFile would
					// throw on the empty path and abort the project open. Skip
					// them at save time so the .slb stays loadable.
					// Splits and duplicates inherit clipRefId via the AudioRegion
					// copy constructors, so this only fires for orphaned regions
					// (e.g. fresh recordings, which currently have no on-disk
					// clip file and therefore can't be reconstructed on load).
					if (ar->getClipRefid().isEmpty() || visibleSamples <= 0) {
						Logger::writeToLog("TrackNavigator: dropping unsaveable region (refId='"
							+ ar->getClipRefid() + "', samples=" + String((juce::int64) visibleSamples) + ")");
						continue;
					}

					region.setProperty("clipRefId", ar->getClipRefid(), nullptr);
					region.setProperty("sampleOffset", (int)ar->getSampleOffset(), nullptr);
					region.setProperty("numSamples", (juce::int64) visibleSamples, nullptr);
					region.setProperty("sourceStartSample", (juce::int64) ar->getSourceStartSample(), nullptr);
					region.setProperty("width", ar->getWidth(), nullptr);
					// Tempo-Follow Status mit serialisieren — wenn followTempo
					// gesetzt ist, stretcht openTracks beim Reload den Buffer
					// nach Bedarf gegen das aktuelle Projekt-Tempo.
					region.setProperty("referenceTempo", ar->getReferenceTempo(), nullptr);
					region.setProperty("followTempo", ar->getFollowTempo(), nullptr);
					// Pitch-Shift-State (Halbtoene). Der gespeicherte Buffer
					// ist BEREITS auf diesen Wert geshiftet, deshalb wird beim
					// Load nur das Member gesetzt und nicht erneut geshiftet.
					region.setProperty("pitch", (double) ar->getPitch(), nullptr);
					// Per-region playback gain (linear, applied in
					// Track::getSample). Buffer remains untouched, so a
					// gain change is a metadata-only operation that survives
					// reload via this property.
					region.setProperty("gain", (double) ar->getGain(), nullptr);
				}
			}
			else if (tracks.at(i)->getType() == Track::MIDI || tracks.at(i)->getType() == Track::INSTRUMENT) {
				if (auto* mr = dynamic_cast<MidiRegion*>(tracks.at(i)->getRegions().at(j))) {
					region.setProperty("sampleOffset", (int)mr->getSampleOffset(), nullptr);
					region.setProperty("numSamples", mr->getNumSamples(), nullptr);
					region.setProperty("sourceStartSample", (juce::int64) mr->getSourceStartSample(), nullptr);
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

		// Automation-Daten persistieren — alle Lanes des Tracks (eine pro
		// Plugin-Parameter, mit stabiler Uuid). Enthaelt auch die Bus-Lanes
		// (paramId "bus:<idx>:<kind>"), die brauchen keine Sonderbehandlung.
		track.addChild(tracks.at(i)->getAutomationModel().toValueTree(), -1, nullptr);

		// UI-Zustand der dedizierten Bus-Automation-Lane: Toggle + gewaehltes
		// Ziel (Bus-Modulindex + BusParam). Die Lane-Punkte stecken bereits im
		// Automation-Child oben; hier nur, welche Lane beim Reload angezeigt /
		// gebunden wird und ob das "B"-Overlay sichtbar ist.
		track.setProperty("busAutoVisible", tracks.at(i)->isBusAutomationVisible(), nullptr);
		track.setProperty("busAutoBus",     (juce::int64) tracks.at(i)->getBusAutomationCurrentBusIndex(), nullptr);
		track.setProperty("busAutoParam",   (int) tracks.at(i)->getBusAutomationCurrentParam(), nullptr);

		// Bus-Sends pro Track. busModuleIndex referenziert Module::getIndex()
		// des BusOut — der ist stabil ueber Save/Load, weil SynthEditor beim
		// Speichern modulIndex mit ins Patch-XML schreibt.
		{
			ValueTree sendsTree("Sends");
			for (const auto& s : tracks.at(i)->getSends()) {
				ValueTree sendNode("Send");
				sendNode.setProperty("busModuleIndex", (juce::int64) s.busModuleIndex, nullptr);
				sendNode.setProperty("amount",         (double)       s.amount,         nullptr);
				sendsTree.addChild(sendNode, -1, nullptr);
			}
			track.addChild(sendsTree, -1, nullptr);
		}

		// FX-Slot-Map (welches Plugin in welchem User-Slot) wird in
		// SynthEditor::saveFile() pro Track angereichert — TrackNavigator
		// hat keinen Zugriff auf MixerPanel/Channel-Strips.

		v.addChild(track, -1, nullptr);
	}

	// Wavelab-style Timeline-Marker auf Projekt-Ebene serialisieren. Liegen
	// nicht in einem Track, deshalb als eigener Child von "Tracks" angehaengt.
	if (timeLine != nullptr) {
		ValueTree markersTree("Markers");
		for (const auto& m : timeLine->getMarkers()) {
			ValueTree mk("Marker");
			mk.setProperty("position",    m.position, nullptr);
			mk.setProperty("name",        m.name,     nullptr);
			mk.setProperty("type",        (m.type == TimelineMarker::Type::Loop) ? "loop" : "normal", nullptr);
			mk.setProperty("loopRepeats", m.loopRepeats, nullptr);
			markersTree.addChild(mk, -1, nullptr);
		}
		v.addChild(markersTree, -1, nullptr);
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

void TrackNavigator::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel)
{
	// Auf macOS wird Ctrl+Scroll vom OS für Accessibility-Zoom abgefangen,
	// daher zusätzlich Cmd akzeptieren.
	const bool isZoomModifier = event.mods.isCtrlDown() || event.mods.isCommandDown();
	const bool isVerticalZoom = isZoomModifier && event.mods.isShiftDown();

	if (isVerticalZoom)
	{
		verticalZoomAtMouse(event, wheel);
		return;
	}

	if (!isZoomModifier || getMaxLength() == 0)
	{
		// Normales Scrollen an den Viewport durchreichen.
		Component::mouseWheelMove(event, wheel);
		return;
	}

	const float oldZoom = this->zoom;

	// Schrittweite analog zur '+' / '-' Taste.
	const float step = (float)(Project::getInstance()->getTempo() / 8.0);
	float delta = wheel.deltaY;
	if (wheel.isReversed) delta = -delta;

	float newZoom = oldZoom + delta * step;
	if (newZoom < 1.0f) newZoom = 1.0f;
	if (newZoom == oldZoom) return;

	// Zeitposition unter der Maus vor dem Zoom merken, damit wir den
	// Viewport hinterher so scrollen können, dass derselbe Punkt unter dem
	// Cursor bleibt.
	const int mouseX = event.getEventRelativeTo(this).x;
	const double timeAtMouse = (double)(mouseX - kControlsWidth) / oldZoom;

	this->zoom = newZoom;
	setSize(getMaxLength() * this->zoom, getHeight());
	setZoom(newZoom);

	if (view != nullptr)
	{
		const int newMouseX = (int)(timeAtMouse * newZoom) + kControlsWidth;
		const int dx = newMouseX - mouseX;
		const auto pos = view->getViewPosition();
		view->setViewPosition(pos.x + dx, pos.y);
	}

	repaint();
	sendChangeMessage();
}

void TrackNavigator::verticalZoomAtMouse(const MouseEvent& event, const MouseWheelDetails& wheel)
{
	if (tracks.empty()) return;

	// Skalierung pro Wheel-Tick. exp() liefert symmetrisches Zoom-In/Out.
	float delta = wheel.deltaY;
	if (wheel.isReversed) delta = -delta;
	const float factor = std::exp(delta * 0.5f);
	if (std::abs(factor - 1.0f) < 1e-4f) return;

	const int mouseY = event.getEventRelativeTo(this).y;

	// Erst den Track unter dem Maus-Cursor finden und seine relative Y-Position
	// in seiner eigenen Bounding-Box festhalten — danach koennen wir die
	// Heights aendern und das Layout reparieren.
	int hostIndex   = -1;
	int hostOffsetY = 0;
	for (int i = 0; i < (int) tracks.size(); ++i)
	{
		auto* t = tracks.at(i);
		const int top = t->getY();
		const int bot = top + t->getHeight();
		if (mouseY >= top && mouseY < bot)
		{
			hostIndex   = i;
			hostOffsetY = mouseY - top;
			break;
		}
	}

	const int kMinTrackHeight = 24;
	const int kMaxTrackHeight = 400;

	for (auto* t : tracks)
	{
		const int newH = juce::jlimit(kMinTrackHeight, kMaxTrackHeight,
		                              (int) std::round(t->getHeight() * factor));
		t->setHeight(newH);
	}

	// Re-Layout (analog zu insertTrackAt).
	int yPos = timelineOffset;
	for (auto* t : tracks)
	{
		t->setTopLeftPosition(kControlsWidth, yPos);
		yPos += t->getHeight();
	}

	syncTrackControls();
	adjustHeight();

	// Viewport so verschieben, dass der Track unter dem Cursor an derselben
	// Bildschirm-Y bleibt.
	if (view != nullptr && hostIndex >= 0 && hostIndex < (int) tracks.size())
	{
		auto* t = tracks.at(hostIndex);
		// Mouse-Position im neuen Layout (im Navigator-Koordinatensystem)
		const float scaledOffset = (float) hostOffsetY * factor;
		const int newMouseY = t->getY() + (int) scaledOffset;
		const int dy = newMouseY - mouseY;
		const auto pos = view->getViewPosition();
		view->setViewPosition(pos.x, pos.y + dy);
	}

	repaint();
	sendChangeMessage();
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

void TrackNavigator::setPlayheadAtX(int x) {
	// x ist relativ zum Navigator (inkl. gescrolltem Bereich). Sample 0 liegt
	// bei x = kControlsWidth, weil davor der TrackControl-Strip sitzt — daher
	// den Offset abziehen, sonst springt der Playhead beim Klick auf den
	// Track-Anfang um den Strip-Breite-Betrag rein.
	double positionInSeconds = (double)(x - kControlsWidth) / zoom;
	if (positionInSeconds < 0.0) positionInSeconds = 0.0;
	setPosition(positionInSeconds);
	marker->setSamplePosition(getSamplePosition());
	// While stopped, manually moving the playhead must not fire notes at the
	// new position. Pass dispatchEvents=false so the track updates its position
	// silently (samplePosition + lastActiveRegion still move so
	// getCurrentRegion/PianoRoll work).
	const bool transportActive = isPlaying() || isRecording();
	for (int i = 0; i < tracks.size(); i++) {
		tracks.at(i)->setSamplePosition(samplePosition, transportActive);
		tracks.at(i)->setCurrentMarkerPosition(position);
	}
	sendChangeMessage();
	marker->repaint();
	// Project::root ist erst gesetzt, sobald SynthEditor seinen Root-Module-
	// Baum gebaut hat. Vor dem ersten setRoot() (oder wenn das Projekt nur
	// Tracks ohne Modular-Baum enthaelt) liefert getRoot() nullptr und der
	// naechste ->getModules() crasht — daher der Guard.
	if (auto* root = Project::getInstance()->getRoot())
	{
		if (auto* mods = root->getModules())
		{
			for (int i = 0; i < (int) mods->size(); i++)
			{
				if (auto* module = dynamic_cast<PluginModule*>(mods->at(i)))
				{
					module->requestPositionReset(samplePosition);
				}
			}
		}
	}
}

void TrackNavigator::invokeMenuCommand(int id) {
	// Routet die Kontextmenue-Auswahl zurueck nach MainComponent::menuItemSelected
	// (von MainComponent als Lambda gesetzt). Ohne Callback (z.B. in Tests) no-op.
	if (onMenuCommand)
		onMenuCommand(id);
}

void TrackNavigator::showRegionContextMenu(Region* region, juce::Point<int> /*screenPos*/) {
	if (region == nullptr)
		return;

	// Rechtsklick selektiert die Region, falls sie noch nicht Teil der
	// Selektion ist — die (selektions-basierten) Menue-Aktionen greifen sonst
	// auf nichts. Eine bestehende Mehrfach-Selektion, die diese Region
	// enthaelt, bleibt erhalten, damit "auf alle Selektierten anwenden" geht.
	if (!region->isSelected()) {
		for (auto* t : tracks) {
			if (t == nullptr) continue;
			t->setSelected(false);
			t->clearSelection();
		}
		region->setSelected(true);
		if (auto* pt = dynamic_cast<Track*>(region->getParentComponent()))
			currentTrack = pt;
		propertyView->selectionChanged(region);
		sendChangeMessage();
		repaint();
	}

	// Item-IDs == die MainComponent-Menue-Command-IDs (siehe
	// MainComponent::menuItemSelected). Keine eigene Logik hier — invokeMenuCommand
	// leitet die Auswahl an den bestehenden Handler weiter.
	juce::PopupMenu menu;
	menu.addItem(43, "Duplicate");
	menu.addItem(44, "Split at Marker");
	menu.addItem(49, "Consolidate");
	menu.addSeparator();
	menu.addItem(71, "Cut");
	menu.addItem(72, "Copy");
	menu.addItem(73, "Paste at Marker");
	menu.addSeparator();

	if (dynamic_cast<AudioRegion*>(region) != nullptr) {
		juce::PopupMenu audio;
		audio.addItem(42, "Detect BPM");
		audio.addItem(46, "Reverse");
		audio.addItem(47, "Normalize");
		audio.addItem(48, "Fade In/Out");
		menu.addSubMenu("Audio", audio);
	}
	else if (dynamic_cast<MidiRegion*>(region) != nullptr) {
		juce::PopupMenu midi;
		midi.addItem(41, "Quantize");
		midi.addItem(74, "Open Piano Roll");
		midi.addItem(75, "Open Message Editor");
		menu.addSubMenu("MIDI", midi);
	}

	menu.addItem(70, "Bounce in Place");
	menu.addSeparator();
	menu.addItem(45, "Delete");

	juce::Component::SafePointer<TrackNavigator> safe(this);
	menu.showMenuAsync(juce::PopupMenu::Options(),
		[safe](int result) {
			if (safe == nullptr || result == 0) return;
			safe->invokeMenuCommand(result);
		});
}

void TrackNavigator::showTrackContextMenu(Track* track, juce::Point<int> /*screenPos*/) {
	// Track unter dem Cursor zum currentTrack machen, damit track-skopierte
	// Aktionen (Delete Track, Add MIDI Clip, Paste at Marker) ihn treffen.
	// track == nullptr bei Rechtsklick auf den leeren Bereich unter den Tracks:
	// dann nur die Add-Track-Items anbieten.
	if (track != nullptr) {
		for (auto* t : tracks)
			if (t != nullptr) t->setSelected(false);
		track->setSelected(true);
		currentTrack = track;
		applyAutoArm(track);
		propertyView->selectionChanged(track);
		sendChangeMessage();
		repaint();
	}

	const bool haveTrack = (track != nullptr);

	juce::PopupMenu menu;
	menu.addItem(8,  "Add Audio Track");
	menu.addItem(9,  "Add MIDI Track");
	menu.addItem(10, "Add Instrument Track");
	menu.addSeparator();
	menu.addItem(11, "Add MIDI Clip", haveTrack);
	menu.addItem(73, "Paste at Marker", haveTrack);
	menu.addSeparator();
	menu.addItem(60, "Delete Track", haveTrack);

	juce::Component::SafePointer<TrackNavigator> safe(this);
	menu.showMenuAsync(juce::PopupMenu::Options(),
		[safe](int result) {
			if (safe == nullptr || result == 0) return;
			safe->invokeMenuCommand(result);
		});
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
    // getRoot() ist nullptr, solange SynthEditor seinen Root-Module-Baum noch
    // nicht gebaut hat bzw. das Projekt nur Tracks ohne Modular-Baum enthaelt —
    // dann darf ->getModules() nicht dereferenziert werden (siehe mouseDown).
    if (auto* root = Project::getInstance()->getRoot()) {
        if (auto* mods = root->getModules()) {
            for (int i = 0; i < (int) mods->size(); i++) {
                if (auto* module = dynamic_cast<PluginModule*>(mods->at(i))) {
                    module->requestPositionReset(0);
                    module->requestReset();
                }
            }
        }
    }

    sendChangeMessage();
}

void TrackNavigator::setSamplePosition(int position) {

	const int oldPos = this->samplePosition.load();
	this->samplePosition = alignToBufferSize(position);

	if (loopEnabled) {
		// Tight loop: Grenzen auf Buffer-Boundaries snappen, sonst frisst das
		// alignToBufferSize oben pro Wrap-Iteration bis zu eine Bufferlaenge an
		// Samples auf -> hoerbarer Stutter. Mit aligned Grenzen trifft der
		// Playhead loopEnd exakt am Block-Ende und dispatcht keine Events mehr
		// nach loopEnd (die sonst sofort vom anschliessenden sendAllNotesOff
		// abgewuergt wuerden). Wir vergleichen mit >=, damit der Wrap genau am
		// Boundary feuert (nicht erst eine Block-Laenge spaeter).
		const double sr = Project::getInstance()->getSampleRate();
		const int    bs = Project::getInstance()->getBufferSize();
		auto alignDown = [bs](long s) { return (int)((s / bs) * bs); };

		const int loopStartSamples = alignDown((long)(loopStart * sr));
		const int loopEndSamples   = alignDown((long)(loopEnd   * sr));

		if (loopEndSamples > loopStartSamples
		    && this->samplePosition.load() >= loopEndSamples)
		{
			for (int i = 0; i < tracks.size(); i++)
				tracks.at(i)->sendAllNotesOff();

			// Falls die Position weit hinter loopEnd liegt (seltener Edge Case,
			// z.B. Seek): mit modulo in den Loop zurueckmappen, statt einfach
			// nur auf loopStart zu klemmen.
			const int loopLen = loopEndSamples - loopStartSamples;
			const int offset  = (this->samplePosition.load() - loopStartSamples) % loopLen;
			this->samplePosition = loopStartSamples + alignDown(offset);
		}
	}

	// Loop-Marker (Wavelab-style): wenn der Playhead waehrend dieses Tick einen
	// Loop-Marker ueberschreitet und der noch Repeats uebrig hat, springen wir
	// zurueck zum vorigen Marker (jeglichen Typs) und dekrementieren. Ohne
	// "previous marker" wird auf 0 gesprungen — entspricht der Wavelab-Logik
	// "wiederhol das Segment vom Anfang bis hier".
	if (playing && timeLine != nullptr) {
		auto& markers = timeLine->getMarkersMutable();
		const double sr = Project::getInstance()->getSampleRate();
		for (auto& m : markers) {
			if (m.type != TimelineMarker::Type::Loop) continue;
			if (m.repeatsLeft <= 0) continue;
			const int ms = (int)(m.position * sr);
			if (oldPos < ms && this->samplePosition.load() >= ms) {
				int prev = 0;
				for (const auto& pm : markers) {
					const int ps = (int)(pm.position * sr);
					if (ps < ms && ps > prev) prev = ps;
				}
				--m.repeatsLeft;
				this->samplePosition = alignToBufferSize(prev);
				for (int i = 0; i < tracks.size(); i++)
					tracks.at(i)->sendAllNotesOff();
				break; // nur einen Marker pro Tick verarbeiten
			}
		}
	}

	for (int i = 0; i < tracks.size(); i++) {
		tracks.at(i)->setSamplePosition(this->getSamplePosition());
	}
    
    // getRoot() liefert nullptr, solange SynthEditor seinen Root-Module-Baum
    // noch nicht gebaut hat bzw. das Projekt nur Tracks ohne Modular-Baum
    // enthaelt (z.B. Audio-Track + User-Modul, bevor setRoot() lief). Der
    // bisherige Code pruefte nur getModules(), nicht getRoot() selbst, und
    // crashte hier auf dem Audio-Thread (siehe mouseDown fuer dasselbe Muster).
    if (auto* root = Project::getInstance()->getRoot()) {
        if (auto* mods = root->getModules()) {
            for (int i = 0; i < (int) mods->size(); i++) {
                if (auto* module = dynamic_cast<PluginModule*>(mods->at(i))) {
                    module->setCurrentSample(this->samplePosition);
                }
            }
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
        // Loop-Marker-Counters fuer den frischen Lauf zuruecksetzen — sonst
        // wuerden bereits "verbrauchte" Marker beim naechsten Play nicht mehr
        // wiederholen.
        if (timeLine != nullptr)
            timeLine->resetLoopCounters();
    }
	else {
		stopTimer();
		allNotesOff();
	}
	Session::getInstance()->setRecordingStartTime();
	// Echte Playhead-Position in Samples (getSamplePosition() ist das, was auch
	// der Audio-Thread liest). Der `position`-Member ist in Sekunden.
	const int startSample = getSamplePosition();

	for (int i = 0; i < tracks.size(); i++) {
		for (int j = 0; j < tracks.at(i)->getRegions().size(); j++) {
			tracks.at(i)->getRegions().at(j)->reset();
			tracks.at(i)->setCurrentMarkerPosition(position);
		}

		if (playing) {
			// Das Startfenster [startSample, startSample+bufferSize) muss HIER
			// gefeuert werden: dispatchMidiMessages → notifyListeners triggert
			// Noten sofort, und der Audio-Callback ruft setSamplePosition immer
			// erst am Block-ENDE mit (getSamplePosition()+numSamples) auf — die
			// erste Position, die der Audio-Thread je dispatcht, ist also
			// startSample+bufferSize. Das Fenster an startSample (mit der Note
			// bei relativer Zeit 0) wuerde sonst NIE dispatcht.
			//
			// forcePositionReset() zuerst, damit der `position != samplePosition`
			// -Guard in Track::setSamplePosition den Dispatch nicht unterdrueckt
			// (track.samplePosition steht sonst evtl. schon auf startSample).
			tracks.at(i)->forcePositionReset();
			tracks.at(i)->setSamplePosition(startSample, true);
		}
		else {
			tracks.at(i)->setSamplePosition(position);
		}
	}
	// getRoot() ist nullptr, solange kein Modular-Baum gesetzt ist — der
	// bisherige Code dereferenzierte ihn ungeprueft und crashte beim Play
	// eines reinen Track-Projekts (gleiches Muster wie mouseDown/setSamplePosition).
	if (auto* root = Project::getInstance()->getRoot()) {
		if (auto* mods = root->getModules()) {
			for (int i = 0; i < (int) mods->size(); i++) {
				if (auto* module = dynamic_cast<PluginModule*>(mods->at(i))) {
					module->getPlayHead()->setRecording(false);
					module->getPlayHead()->setPlaying(playing);
					if (playing) {
						module->requestPositionReset(this->samplePosition);
						module->requestReset();
					}
				}
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
			Track* t = tracks.at(i);
			if (t->isRecording()) {

				if (t->getType() == Track::Type::MIDI || t->getType() == Track::Type::INSTRUMENT) {

					MidiRegion* r = dynamic_cast<MidiRegion*>(t->getRegionAt(recordStart));
					if (r == nullptr) {
						r = t->addMidiRegion(Project::getInstance()->getSampleRate(), recordStart, Project::getInstance()->getSampleRate() * 10);
					}
					t->setRecordingRegion(r);
					r->startRecording();
				}
				else if (t->getType() == Track::Type::AUDIO) {
					AudioRegion* r = dynamic_cast<AudioRegion*>(t->getRegionAt(recordStart));
					if (r == nullptr) {
						r = t->addRegion(nullptr, Project::getInstance()->getSampleRate(), recordStart, Project::getInstance()->getSampleRate() * 10);
					}
					t->setRecordingRegion(r);
					r->startRecording();
				}

			}
		}
	}
	else {
		stopTimer();
		Session::getInstance()->setRecordingStopTime();
		recordStop = marker->getSamplePosition() * Project::getInstance()->getSampleRate();

		for (int i = 0; i < tracks.size(); i++) {
			Track* t = tracks.at(i);
			if (t->isRecording()) {
				Region* rec = t->getRecordingRegion();
				if (t->getType() == Track::Type::MIDI || t->getType() == Track::Type::INSTRUMENT) {
					if (auto* mr = dynamic_cast<MidiRegion*>(rec)) {
						mr->stopRecording();
						mr->sanitize();
					}
				}
				else if (t->getType() == Track::Type::AUDIO) {
					if (auto* ar = dynamic_cast<AudioRegion*>(rec)) {
						ar->stopRecording();
						// Recording-Buffer als WAV nach $project/recordings
						// persistieren und der Region einen clipRefId geben —
						// sonst wirft Project::save() die Region weg (siehe
						// "dropping unsaveable region"-Pfad). Spiegel zur
						// Bounce-in-Place-Logik weiter unten.
						persistRecordedRegion(t, ar, recordStart);
					}
				}
				t->setRecordingRegion(nullptr);
			}
		}
	}

	this->recording = recording;
}

bool TrackNavigator::isRecording() {
	return recording;
}

void TrackNavigator::togglePlayStop() {
	if (isPlaying() || isRecording()) {
		if (isRecording())
			setRecording(false);
		setPlaying(false);
	}
	else {
		setPlaying(true);
	}
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
	// Editor + Callbacks lokal aufbauen, dann ans MainComponent ueberreichen.
	// Owner wird ab dem createPanel-Aufruf das DockablePanel "pianoroll";
	// wir behalten nur eine SafePointer-Referenz fuer Transport-Sync im
	// Timer-Callback und fuer den clearTracks-Cleanup-Pfad.
	auto editor = std::make_unique<PianoRollEditor>(r, Project::getInstance()->getSampleRate());
	auto* editorPtr = editor.get();

	// Projekt-Tempo an die Piano Roll uebergeben. Ohne das laeuft der
	// interne Default (120 BPM) gegen das Projekt — bei z.B. 60 BPM
	// fallen Bar-Linien doppelt so dicht, eine Note an Sample 1*Bar
	// landet auf der zweiten Bar-Linie und der User liest sie als
	// "Takt 3" statt "Takt 2". (Tempo-Aenderungen waehrend die Roll
	// offen ist: zurzeit nicht live propagiert — Roll neu oeffnen.)
	editorPtr->getRoll().setBPM(Project::getInstance()->getTempo());

	editorPtr->getRoll().setGridResolution(16);

	editorPtr->getRoll().onPlayToggle = [this]() {
		togglePlayStop();
	};

	// samplePos kommt aus PianoRollComponent::getTimeAtX() und ist BUFFER-
	// relativ (gleicher Bezug wie midiMessageList-Timestamps). setPosition()
	// erwartet aber eine TIMELINE-Position — also zurueckkonvertieren:
	//   timeline = (buffer - sourceStartSample) + sampleOffset
	// Ohne das springt der globale Marker beim Klick in die Piano-Roll auf
	// 0 (statt auf sampleOffset), und die Marker-Position passt nicht mehr
	// zu den Noten, neben die der User geklickt hat.
	editorPtr->getRoll().onMarkerPositionChanged = [this, r](double samplePos) {
		const double offsetSample = (r != nullptr) ? (double) r->getSampleOffset() : 0.0;
		const double base         = (r != nullptr) ? (double) r->getSourceStartSample() : 0.0;
		const double timelineSamples = samplePos - base + offsetSample;
		double posInSeconds = timelineSamples / Project::getInstance()->getSampleRate();
		setPosition(posInSeconds);
		for (int i = 0; i < tracks.size(); i++) {
			tracks.at(i)->setSamplePosition(this->samplePosition);
			tracks.at(i)->setCurrentMarkerPosition(this->position);
		}
		sendChangeMessage();
	};

	// Interaktive Piano-Leiste: Klick spielt die Note ueber den gleichen Pfad
	// wie externe MIDI-Hardware (MidiProcessor::handleIncomingMidiMessage),
	// damit Routing und aktiver Kanal automatisch passen. Kanal kommt vom
	// aktuell selektierten Track (Fallback: 1).
	editorPtr->getRoll().onPianoKeyPlayed = [this](int note, int velocity, bool noteOn) {
		auto* mp = Project::getInstance()->getMidiProcessor();
		if (mp == nullptr) return;
		Track* current = getCurrentTrack();
		const int channel = (current != nullptr) ? current->getMidiChannel() : 1;
		auto msg = noteOn
			? juce::MidiMessage::noteOn(channel, note, (juce::uint8) velocity)
			: juce::MidiMessage::noteOff(channel, note);
		mp->handleIncomingMidiMessage(nullptr, msg);
	};

	// SafePointer VOR move() setzen — danach besitzt der unique_ptr-Move
	// das Original; wir wollen aber weiter via editorPtr darauf zugreifen,
	// nachdem das DockablePanel ihn aufgenommen hat (raw pointer bleibt
	// gueltig, das Component-Lifetime managed jetzt das Panel).
	pianoRoll = editorPtr;

	if (auto* mc = findParentComponentOfClass<MainComponent>())
		mc->showOrCreatePianoRollPanel(std::move(editor));
}

void TrackNavigator::openEventEditor(MidiRegion* r)
{
	// Falls eine Piano-Roll offen ist: schliesse sie ueber den Dock-Pfad —
	// das einfache pianoRoll=nullptr wuerde nur die SafePointer leeren, das
	// DockablePanel mit verwaistem Editor bliebe stehen.
	if (pianoRoll != nullptr) {
		if (auto* mc = findParentComponentOfClass<MainComponent>())
			mc->closePianoRollPanel();
		pianoRoll = nullptr;
	}

	// Event-Editor als eigenes Dock-Panel — selbes Muster wie Piano-Roll.
	// Owner ab createPanel ist das DockablePanel "eventeditor".
	auto editor = std::make_unique<MidiRegionEventEditor>(r, Project::getInstance()->getSampleRate());
	if (auto* mc = findParentComponentOfClass<MainComponent>())
		mc->showOrCreateEventEditorPanel(std::move(editor));
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
	marker->setZoom(zoom);

	repaint();
}

void TrackNavigator::ensureDefaultAudioTracks(int n) {
	// Fuellt fehlende Audio-Tracks auf, damit ein leeres Projekt sofort
	// brauchbar ist (DAW-typisch). Ueber TrackUtil::createAudioTrack, damit
	// dieselbe Verdrahtung passiert wie bei manuell angelegten Tracks.
	if (editor == nullptr || editor->getModule() == nullptr) return;
	while ((int) tracks.size() < n) {
		Track* t = TrackUtil::createAudioTrack(editor, this, editor->getModule());
		if (t == nullptr) break;   // Schutz, falls Anlage fehlschlaegt
	}

	// Undo-Stack nach dem Default-Setup leeren: die N AddModuleActions aus
	// TrackUtil sind nichts, was der User je rueckgaengig machen will, und
	// halbe Default-States (z.B. nur 1 von 2 Tracks "weg-undo'd") fuehren zu
	// seltsamem Verhalten in der UI.
	if (auto* um = Project::getInstance()->getUndoManager())
		um->clearUndoHistory();
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

Track* TrackNavigator::detachTrack(Track* target, int& outIndex)
{
	if (target == nullptr) return nullptr;
	for (int i = 0; i < (int)tracks.size(); i++) {
		if (tracks[i] != target) continue;

		outIndex = i;
		tracks.erase(tracks.begin() + i);
		target->removeChangeListener(this);
		removeChildComponent(target);

		// Restack remaining tracks (mirror detachSelectedTrack).
		if (!tracks.empty()) {
			tracks[0]->setTopLeftPosition(tracks[0]->getX(), timelineOffset);
			for (int j = 1; j < (int)tracks.size(); j++)
				tracks[j]->setTopLeftPosition(tracks[j-1]->getX(),
				                              tracks[j-1]->getY() + tracks[j-1]->getHeight());
		}

		if (currentTrack == target)
			currentTrack = tracks.empty() ? nullptr : tracks[0];

		syncTrackControls();
		adjustHeight();
		repaint();
		sendChangeMessage();
		return target;
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
	// Timeline vertikal am sichtbaren oberen Rand halten (siehe moved()).
	this->timeLine->setTopLeftPosition(kControlsWidth, juce::jmax(0, -getY()));

	syncTrackControls();
}

void TrackNavigator::changeListenerCallback(ChangeBroadcaster* source)
{
	updateTrackLayout(source);

	if (source == Project::getInstance()) {
		tracklength = Project::getInstance()->getTrackLength();

		for (int i = 0; i < tracks.size(); i++) {
			tracks.at(i)->setTrackLength(tracklength);
		}

		// Tempo-Wechsel? AudioRegions mit followTempo=true via Rubberband
		// stretchen. AudioRegion::applyTempoStretch ist no-op fuer Regionen
		// ohne followTempo, ist also safe pauschal aufzurufen. Erster
		// Callback (lastKnownTempo == 0) ist kein "Wechsel" — wir
		// initialisieren nur den Cache.
		const float currentTempo = Project::getInstance()->getTempo();
		juce::Logger::writeToLog("TrackNavigator: Project change callback. tempo="
			+ juce::String(currentTempo) + " lastKnown=" + juce::String(lastKnownTempo));
		if (lastKnownTempo > 0.0f
			&& std::abs(currentTempo - lastKnownTempo) > 0.001f)
		{
			const double sr = Project::getInstance()->getSampleRate();
			juce::Logger::writeToLog("TrackNavigator: tempo change detected, walking regions");
			int audioRegionCount = 0;
			for (auto* t : tracks) {
				if (t == nullptr || t->getType() != Track::AUDIO) continue;
				for (auto* r : t->getRegions()) {
					if (auto* ar = dynamic_cast<AudioRegion*>(r)) {
						++audioRegionCount;
						ar->applyTempoStretch((double) currentTempo, sr);
					}
				}
			}
			juce::Logger::writeToLog("TrackNavigator: visited " + juce::String(audioRegionCount) + " AudioRegions");
			repaint();
		}
		lastKnownTempo = currentTempo;
	}
	if (Track* track = dynamic_cast<Track*>(source)) {
		sendChangeMessage();
	}

}

bool TrackNavigator::keyPressed(const KeyPress& key, Component* originatingComponent) {

	// hasFocus-Gate nur fuer modifier-LOSE Tasten anwenden — sonst gehen im
	// Docking-Layout alle Cmd+T/D/C/V/X-Shortcuts verloren, weil der Fokus
	// fast immer in einem anderen Dock-Panel (Editor, Mixer, leerer Tab)
	// liegt und der Navigator focusGained() nie sieht. Modifier-Tastendrucke
	// wie Cmd+X kollidieren nicht mit Text-Eingaben und sollen global
	// triggern, solange der Top-Level Key-Listener sie an uns durchreicht.
	// Reine Tasten (Space, +/-) bleiben fokus-gated, sonst wuerde z.B.
	// Space waehrend einer Texteingabe Play/Stop togglen.
	const bool hasModifier = key.getModifiers().isCommandDown()
	                      || key.getModifiers().isAltDown()
	                      || key.getModifiers().isCtrlDown();
	if (!hasFocus && !hasModifier) {
		return false;
	}

	if (key == KeyPress::spaceKey) {
		togglePlayStop();
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

	// Untergrenze für Zoom — sonst fällt zoom ggf. <= 0 und das Layout kollabiert.
	if (this->zoom < 1.0f) this->zoom = 1.0f;

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
			if (Track* cur = getCurrentTrack())
				if (auto* mr = dynamic_cast<MidiRegion*>(cur->getCurrentRegion()))
					openPianoRoll(mr);
		}
		else if (key.getKeyCode() == 77) { // M;
			if (Track* cur = getCurrentTrack())
				if (auto* mr = dynamic_cast<MidiRegion*>(cur->getCurrentRegion()))
					openEventEditor(mr);
		}
		else if (key.getKeyCode() == 73) { // I
			for (int i = 0; i < tracks.size(); i++) {
				for (int j = 0; j < tracks.at(i)->getRegions().size(); j++) {
					Logger::getCurrentLogger()->writeToLog(String(tracks.at(i)->getRegions().at(j)->getSampleOffset()));
				}
			}
		}
		else if (key.getKeyCode() == 88) { // X — standard cut: copy to clipboard, then remove.
			Session::getInstance()->clearRegionsClipboard();
			for (int i = 0; i < tracks.size(); i++) {
				tracks.at(i)->copySelectedRegions();
			}
			for (int i = 0; i < tracks.size(); i++) {
				tracks.at(i)->removeSelectedRegions(true);
			}
		}
		else if (key.getKeyCode() == 84) { // T
			for (int i = 0; i < tracks.size(); i++) {
				tracks.at(i)->splitRegion();
			}
		}
		else if (key.getKeyCode() == 66) { // B — Bounce-In-Place der selektierten Regionen
			bounceSelectedRegionsInPlace();
		}
	}

	return false;
}

void TrackNavigator::persistRecordedRegion(Track* track, AudioRegion* region, long startOffset)
{
	if (track == nullptr || region == nullptr) return;
	auto* buf = region->getBuffer();
	if (buf == nullptr || buf->getNumSamples() <= 0) return;

	const double sr = Project::getInstance()->getSampleRate();
	if (sr <= 0.0) return;

	auto sanitize = [](juce::String s) {
		return s.replaceCharacters(" /\\:*?\"<>|\t\n\r", "_____________");
	};

	// Output-Verzeichnis: bevorzugt $projectDir/recordings, sonst
	// ~/Documents/Synthlab/recordings als Fallback fuer Untitled-Sessions.
	juce::File outputDir;
	const juce::String curProjPath = Project::getInstance()->getCurrentFilePath();
	if (curProjPath.isNotEmpty() && ! Project::getInstance()->isNewFile()) {
		juce::File parent = juce::File(curProjPath).getParentDirectory();
		if (parent.exists())
			outputDir = parent.getChildFile("recordings");
	}
	if (outputDir == juce::File())
		outputDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
			.getChildFile("Synthlab").getChildFile("recordings");
	if (! outputDir.exists())
		outputDir.createDirectory();

	// Zeit im Filename = Timeline-Position des Aufnahmebeginns (MM-SS-mmm).
	// Mehrfaches Aufnehmen an gleicher Position kollidiert sonst — getNonexistentSibling
	// haengt _2, _3 ... an.
	const double seconds = (double) startOffset / sr;
	const int totalMs = (int) std::llround(seconds * 1000.0);
	const int mm  = totalMs / 60000;
	const int ss  = (totalMs / 1000) % 60;
	const int mss =  totalMs % 1000;
	const juce::String timeStr = juce::String::formatted("%02dm%02ds%03d", mm, ss, mss);
	const juce::String baseName = sanitize(track->getName()) + "_rec_" + timeStr;
	const juce::File outFile = outputDir.getChildFile(baseName + ".wav").getNonexistentSibling();
	const juce::String refId = outFile.getFileNameWithoutExtension();

	bool wrote = false;
	{
		juce::WavAudioFormat wavFormat;
		std::unique_ptr<juce::FileOutputStream> stream(outFile.createOutputStream());
		if (stream != nullptr) {
			std::unique_ptr<juce::AudioFormatWriter> writer(
				wavFormat.createWriterFor(stream.get(), sr,
					(unsigned int) juce::jmax(1, buf->getNumChannels()), 16, {}, 0));
			if (writer != nullptr) {
				stream.release(); // ownership -> writer
				wrote = writer->writeFromAudioSampleBuffer(*buf, 0, buf->getNumSamples());
			}
		}
		if (! wrote)
			Logger::writeToLog("Recording-Persist: konnte WAV nicht schreiben: " + outFile.getFullPathName());
	}

	if (wrote) {
		Project::getInstance()->addAudioFile(refId, outFile.getFullPathName());
		region->setClipRefId(refId);
		Logger::writeToLog("Recording-Persist: " + outFile.getFullPathName() + " (refId=" + refId + ")");
	}
}

void TrackNavigator::persistDirtyAudioRegions(const juce::File& outputDirIn)
{
	const double sr = Project::getInstance()->getSampleRate();
	if (sr <= 0.0) return;

	juce::File outputDir = outputDirIn;
	if (outputDir == juce::File())
		outputDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
			.getChildFile("Synthlab").getChildFile("edits");
	if (! outputDir.exists())
		outputDir.createDirectory();

	auto sanitize = [](juce::String s) {
		return s.replaceCharacters(" /\\:*?\"<>|\t\n\r", "_____________");
	};

	for (auto* t : tracks) {
		if (t == nullptr) continue;
		for (auto* r : t->getRegions()) {
			auto* ar = dynamic_cast<AudioRegion*>(r);
			if (ar == nullptr) continue;
			if (! ar->isBufferMutated()) continue;

			auto* buf = ar->getBuffer();
			if (buf == nullptr || buf->getNumSamples() <= 0) continue;

			// Timestamp = Timeline-Position der Region (MM-SS-mmm). Mehrfaches
			// Editieren derselben Region erzeugt sonst Namen-Kollisionen, die
			// wir mit getNonexistentSibling() (_2, _3, ...) aufloesen.
			const long offset = ar->getSampleOffset();
			const double seconds = (double) offset / sr;
			const int totalMs = (int) std::llround(seconds * 1000.0);
			const int mm  = totalMs / 60000;
			const int ss  = (totalMs / 1000) % 60;
			const int mss =  totalMs % 1000;
			const juce::String timeStr = juce::String::formatted("%02dm%02ds%03d", mm, ss, mss);
			const juce::String baseName = sanitize(t->getName()) + "_edit_" + timeStr;
			const juce::File outFile = outputDir.getChildFile(baseName + ".wav").getNonexistentSibling();
			const juce::String refId = outFile.getFileNameWithoutExtension();

			bool wrote = false;
			{
				juce::WavAudioFormat wavFormat;
				std::unique_ptr<juce::FileOutputStream> stream(outFile.createOutputStream());
				if (stream != nullptr) {
					std::unique_ptr<juce::AudioFormatWriter> writer(
						wavFormat.createWriterFor(stream.get(), sr,
							(unsigned int) juce::jmax(1, buf->getNumChannels()), 16, {}, 0));
					if (writer != nullptr) {
						stream.release();
						wrote = writer->writeFromAudioSampleBuffer(*buf, 0, buf->getNumSamples());
					}
				}
			}

			if (wrote) {
				Project::getInstance()->addAudioFile(refId, outFile.getFullPathName());
				ar->setClipRefId(refId);
				ar->markBufferClean();
				Logger::writeToLog("Edit-Persist: " + outFile.getFullPathName() + " (refId=" + refId + ")");
			}
			else {
				Logger::writeToLog("Edit-Persist: konnte WAV nicht schreiben: " + outFile.getFullPathName());
			}
		}
	}
}

void TrackNavigator::bounceSelectedRegionsInPlace()
{
	// Pro selektierter Region: bouncen, neuen Audio-Track DIREKT unter dem
	// Source-Track anlegen, AudioRegion an gleicher Timeline-Position
	// einsetzen. Source-Region bleibt unangetastet.
	if (editor == nullptr) return;

	// Snapshot von (sourceTrack, region)-Paaren, weil wir gleich tracks
	// modifizieren — Iterieren waehrend insertTrackAt() ist unsicher.
	//
	// Achtung: es gibt zwei parallele Region-Selection-Zustaende:
	//   - Region::selected (Flag) — gesetzt von Region::setSelected(),
	//     ausgewertet von isSelected(). Wird beim Klick auf den Navigator-
	//     Hintergrund (TrackNavigator::mouseDown) sauber gepflegt.
	//   - MultiComponentDragger::selectedComponents — visueller Drag-State,
	//     gesetzt wenn der User direkt auf eine Region klickt
	//     (Region::mouseDown -> dragger->handleMouseDown). Dieser Pfad
	//     synchronisiert das Region-Flag NICHT.
	//
	// Frueher hat dieser Loop nur isSelected() gelesen — und damit Direct-
	// Klicks ignoriert, was als "Bounce in Place tut nichts" auffiel. Wir
	// akzeptieren beide Selection-Quellen.
	std::vector<std::pair<Track*, Region*>> jobs;
	for (auto* t : tracks) {
		if (t == nullptr) continue;
		for (auto* r : t->getRegions()) {
			if (r == nullptr) continue;
			const bool flagSel    = r->isSelected();
			const bool draggerSel = (dragger != nullptr && dragger->isSelected(r));
			if (flagSel || draggerSel)
				jobs.emplace_back(t, r);
		}
	}

	const double sr = Project::getInstance()->getSampleRate();
	for (auto& job : jobs) {
		Track* sourceTrack = job.first;
		Region* sourceRegion = job.second;
		if (sourceTrack == nullptr || sourceRegion == nullptr) continue;

		const long offset = sourceRegion->getSampleOffset();
		const long len    = (long) sourceRegion->getNumSamples();

		std::unique_ptr<juce::AudioSampleBuffer> rendered (sourceTrack->bounceRegionToBuffer(sourceRegion));
		if (rendered == nullptr) continue;

		// Gerenderten Buffer als .wav auf Platte schreiben und unter einem
		// stabilen refId in Project::projectAudio registrieren. Ohne diesen
		// Schritt droppt getConfiguration() die Region beim Save (siehe
		// "dropping unsaveable region"-Pfad), weil clipRefId leer waere.
		// Der refId landet automatisch in _audio.xml (Project::save() ->
		// ProjectSnapshot::audioFiles), die Track-XML referenziert ihn ueber
		// region.clipRefId.
		auto sanitize = [](juce::String s) {
			return s.replaceCharacters(" /\\:*?\"<>|\t\n\r", "_____________");
		};

		juce::File outputDir;
		const juce::String curProjPath = Project::getInstance()->getCurrentFilePath();
		if (curProjPath.isNotEmpty() && ! Project::getInstance()->isNewFile()) {
			juce::File parent = juce::File(curProjPath).getParentDirectory();
			if (parent.exists())
				outputDir = parent.getChildFile("bounces");
		}
		if (outputDir == juce::File())
			outputDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
				.getChildFile("Synthlab").getChildFile("bounces");
		if (! outputDir.exists())
			outputDir.createDirectory();

		// "Zeit" im Filename = Timeline-Position der Source-Region (MM-SS-mmm),
		// nicht Wall-Clock, damit der Name musikalisch ablesbar bleibt.
		// Kollisionen (mehrfaches Bouncen derselben Position) loest
		// getNonexistentSibling().
		const double seconds = sr > 0.0 ? ((double) offset / sr) : 0.0;
		const int totalMs = (int) std::llround(seconds * 1000.0);
		const int mm  = totalMs / 60000;
		const int ss  = (totalMs / 1000) % 60;
		const int mss =  totalMs % 1000;
		const juce::String timeStr = juce::String::formatted("%02dm%02ds%03d", mm, ss, mss);
		const juce::String baseName = sanitize(sourceTrack->getName()) + "_bounce_" + timeStr;
		const juce::File outFile = outputDir.getChildFile(baseName + ".wav").getNonexistentSibling();
		const juce::String refId = outFile.getFileNameWithoutExtension();

		bool wrote = false;
		{
			juce::WavAudioFormat wavFormat;
			std::unique_ptr<juce::FileOutputStream> stream (outFile.createOutputStream());
			if (stream != nullptr) {
				std::unique_ptr<juce::AudioFormatWriter> writer (
					wavFormat.createWriterFor(stream.get(), sr,
						(unsigned int) juce::jmax(1, rendered->getNumChannels()), 16, {}, 0));
				if (writer != nullptr) {
					stream.release(); // ownership -> writer
					wrote = writer->writeFromAudioSampleBuffer(*rendered, 0, rendered->getNumSamples());
				}
			}
			if (! wrote)
				Logger::writeToLog("Bounce-in-Place: konnte WAV nicht schreiben: " + outFile.getFullPathName());
		}

		if (wrote)
			Project::getInstance()->addAudioFile(refId, outFile.getFullPathName());

		// Neuen Audio-Track anlegen — TrackUtil haengt ihn ans Ende
		// der tracks-Liste.
		Track* newAudio = TrackUtil::createAudioTrack(editor, this, editor->getModule());
		if (newAudio == nullptr) continue;
		newAudio->setName(sourceTrack->getName() + " (bounce)");

		// In die Position direkt unterhalb des Source-Tracks verschieben.
		const auto srcIt = std::find(tracks.begin(), tracks.end(), sourceTrack);
		const auto newIt = std::find(tracks.begin(), tracks.end(), newAudio);
		if (srcIt != tracks.end() && newIt != tracks.end() && srcIt + 1 != newIt) {
			tracks.erase(newIt);
			const auto refresh = std::find(tracks.begin(), tracks.end(), sourceTrack);
			tracks.insert(refresh + 1, newAudio);

			// Re-Layout (analog zu insertTrackAt).
			int yPos = timelineOffset;
			for (auto* tt : tracks) {
				tt->setTopLeftPosition(kControlsWidth, yPos);
				yPos += tt->getHeight();
			}
			syncTrackControls();
			adjustHeight();
			repaint();
			sendChangeMessage();
		}

		// AudioRegion mit gerendertem Buffer auf den neuen Track. clipRefId
		// nur setzen, wenn der WAV-Write tatsaechlich erfolgreich war —
		// sonst wuerde getConfiguration() einen refId persistieren, dessen
		// Datei nicht existiert, und der Reload wuerde fehlschlagen.
		AudioRegion* newRegion = newAudio->addRegion(rendered.get(), sr, offset, len);
		if (newRegion != nullptr && wrote) {
			newRegion->setClipRefId(refId);
			newRegion->setName(refId);
		}
	}
}

int TrackNavigator::alignToBufferSize(int samplePosition) {
	int bufferSize = Project::getInstance()->getBufferSize();
	return (samplePosition / bufferSize) * bufferSize;
}

void TrackNavigator::mouseUp(const MouseEvent& event) {
	if (marqueeActive) {
		finalizeMarqueeSelection();
		marqueeActive = false;
		marqueeArmed = false;
		marqueeRect = {};
		repaint();
		return;
	}
	marqueeArmed = false;
}

void TrackNavigator::mouseDrag(const MouseEvent& event) {
	if (isPanning)
		return;

	// (frueheres Alt+LeftDrag → WaveSelector-Range: Feature komplett entfernt,
	// hatte keinen Consumer und hinterliess nur das cyan-Overlay-Artefakt.
	// Wave-Editing kommt spaeter in einem eigenen UI.)

	// Marquee: erst nachdem die Maus weit genug bewegt wurde aktivieren —
	// einzelne Klicks (mit minimalem Wackeln) sollen weiterhin als Klick
	// gelten und nicht versehentlich die Selektion umbauen.
	if (marqueeArmed && event.mods.isLeftButtonDown()) {
		if (!marqueeActive) {
			if (event.getDistanceFromDragStart() < 4)
				return;
			marqueeActive = true;
			if (!marqueeAdditive && dragger != nullptr)
				dragger->deselectAll();
		}
		const auto p = event.getEventRelativeTo(this).getPosition();
		marqueeRect = juce::Rectangle<int>(marqueeDownPos, p);
		repaint();
	}
}

void TrackNavigator::paintOverChildren(juce::Graphics& g) {
	// Positions-Marker UEBER den Regions zeichnen. Exakt die gleiche Sample->X-
	// Abbildung wie Tracks/Timeline (navigatorXForSample = kControlsWidth +
	// sample*zoom/sr), damit er zur echten Wiedergabeposition passt und nicht
	// hinter den Regions verschwindet.
	{
		const int markerX = navigatorXForSample((long) marker->getSamplePosition());
		g.setColour(juce::Colours::green);
		g.fillRect(markerX, 0, 2, getHeight());
	}

	// Drag&Drop-Vorschau: halbtransparentes Rechteck an der grid-gesnappten
	// Position, an der die Region beim Loslassen landen wuerde.
	if (dragPreviewActive) {
		auto r = dragPreviewRect.toFloat().reduced(0.0f, 2.0f);
		g.setColour(juce::Colours::aqua.withAlpha(0.25f));
		g.fillRoundedRectangle(r, 6.0f);
		g.setColour(juce::Colours::aqua.withAlpha(0.9f));
		g.drawRoundedRectangle(r, 6.0f, 1.5f);
		// Snap-Linie an der Startkante hervorheben.
		g.fillRect((float)dragPreviewRect.getX() - 1.0f, (float)dragPreviewRect.getY(),
		           2.0f, (float)dragPreviewRect.getHeight());
	}

	// Empty-State-Hint entfernt — leeres Projekt bekommt 2 Default-Audio-Tracks
	// (siehe ensureDefaultAudioTracks). DAW-typisch und loest das Drop-Problem
	// ohne Hint-Text.
	if (!marqueeActive) return;
	g.setColour(juce::Colours::aqua.withAlpha(0.18f));
	g.fillRect(marqueeRect);
	g.setColour(juce::Colours::aqua);
	g.drawRect(marqueeRect, 1);
}

void TrackNavigator::finalizeMarqueeSelection() {
	if (dragger == nullptr) return;
	const juce::Rectangle<int> sel = marqueeRect;
	if (sel.isEmpty()) return;

	for (auto* t : tracks) {
		if (t == nullptr) continue;
		for (auto* r : t->getRegions()) {
			if (r == nullptr) continue;
			// Region-Bounds in Navigator-Koordinaten — Region liegt in Track,
			// Track im Navigator. getLocalArea konvertiert ueber die Hierarchie.
			const auto rb = getLocalArea(r, r->getLocalBounds());
			if (sel.intersects(rb))
				dragger->setSelected(r, true);
		}
	}

	for (auto* t : tracks)
		if (t != nullptr) t->repaint();
}

void TrackNavigator::handlePanMouseDown(const MouseEvent& event)
{
	if (!event.mods.isMiddleButtonDown())
		return;

	isPanning = true;
	panStartScreenPos = event.getScreenPosition();
	panStartViewPos = (view != nullptr) ? view->getViewPosition() : juce::Point<int>(0, 0);
	setMouseCursor(juce::MouseCursor::DraggingHandCursor);
}

void TrackNavigator::handlePanMouseDrag(const MouseEvent& event)
{
	if (!isPanning || view == nullptr)
		return;

	const int dx = event.getScreenPosition().x - panStartScreenPos.x;
	const int newX = juce::jmax(0, panStartViewPos.x - dx);
	view->setViewPosition(newX, panStartViewPos.y);
}

void TrackNavigator::handlePanMouseUp(const MouseEvent& event)
{
	if (!isPanning)
		return;

	isPanning = false;
	setMouseCursor(juce::MouseCursor::NormalCursor);
}

void TrackNavigator::mouseMove(const juce::MouseEvent& event) {

}

void TrackNavigator::mouseDown(const MouseEvent& event) {

	// event.x ist relativ zur Komponente, die geklickt wurde
	// Wir brauchen die Position relativ zum Navigator
	int x = event.getEventRelativeTo(this).x;
	int y = event.getEventRelativeTo(this).y;

	// Marquee nur, wenn auf leerem Hintergrund oder einem Track-Hintergrund
	// gedrueckt wurde — auf einer Region (oder einem Kind davon, z.B. den
	// Resize-Handles am linken/rechten Rand) soll der bestehende Region-Drag
	// bzw. Resize weiter wie gewohnt arbeiten. Ohne Kinder-Check feuert das
	// Marquee beim Klick auf den 5-px-Linksresizer mit, deselektiert die
	// Region und macht das Trim-Drag visuell wie einen Move-Drag.
	// Loop-Range (Alt), Pan (Mitte) und Playhead-Set (Rechts) blocken
	// das Marquee ebenfalls.
	const bool onRegionOrChild =
		(event.eventComponent != nullptr)
		&& (dynamic_cast<Region*>(event.eventComponent) != nullptr
			|| event.eventComponent->findParentComponentOfClass<Region>() != nullptr);
	marqueeArmed =
		event.mods.isLeftButtonDown()
		&& !event.mods.isAltDown()
		&& !event.mods.isRightButtonDown()
		&& !event.mods.isMiddleButtonDown()
		&& !onRegionOrChild;
	marqueeActive = false;
	// Additiv = nur Cmd. Shift ist jetzt dem Positionsmarker vorbehalten und
	// erweitert die Selektion nicht mehr (sonst doppelt belegt).
	marqueeAdditive = event.mods.isCommandDown();
	marqueeDownPos = { x, y };
	marqueeRect = {};

	// Rechtsklick (ohne Ctrl) -> Kontextmenue fuer das getroffene Objekt.
	// Frueh raus, damit weder Marquee, Playhead noch die Selektions-Pfade
	// weiter unten fuer den Rechtsklick mitlaufen. Der Handler feuert auch
	// fuer Klicks auf Regions/Tracks (Navigator ist Mouse-Listener auf allen
	// verschachtelten Kindern), eventComponent sagt uns also, was getroffen
	// wurde.
	if (!event.mods.isCtrlDown() && event.mods.isRightButtonDown()) {
		Region* clickedRegion = dynamic_cast<Region*>(event.eventComponent);
		if (clickedRegion == nullptr && event.eventComponent != nullptr)
			clickedRegion = event.eventComponent->findParentComponentOfClass<Region>();

		if (clickedRegion != nullptr) {
			showRegionContextMenu(clickedRegion, event.getScreenPosition());
		}
		else {
			Track* clickedTrack = dynamic_cast<Track*>(event.eventComponent);
			if (clickedTrack == nullptr && event.eventComponent != nullptr)
				clickedTrack = event.eventComponent->findParentComponentOfClass<Track>();
			showTrackContextMenu(clickedTrack, event.getScreenPosition());
		}
		return;
	}

	// Linksklick (ohne Alt) setzt den Positionsmarker. Weil dieser Handler
	// auch fuer Klicks auf Regions/Tracks feuert, gilt das fuer JEDEN
	// Linksklick im Arrange-Bereich (so vom User gewuenscht). Alt bleibt fuer
	// Spezial-Drags reserviert (Loop-Range, Sampler-Export) und bewegt den
	// Marker nicht.
	if (event.mods.isLeftButtonDown() && !event.mods.isAltDown()) {
		// Nur beim tatsaechlichen Umsetzen des Playheads die hängenden Noten
		// killen — NICHT mehr bei jedem Klick, sonst setzt die Wiedergabe bei
		// reiner Selektion/Editing aus.
		if (isPlaying())
			allNotesOff();
		setPlayheadAtX(x);
	}

	//else
	if (Track* r = dynamic_cast<Track*>(event.eventComponent)) {
		for (int i = 0; i < tracks.size(); i++) {
			tracks.at(i)->setSelected(false);
			// Bei Shift/Cmd Region-Selektion behalten — sonst wuerde Marquee
			// im additiven Modus durch diesen Klick-Pfad sofort entleert.
			if (!marqueeAdditive)
				tracks.at(i)->clearSelection();
		}

		r->setSelected(true);
		currentTrack = r;
		deviceManager->setDefaultMidiOutput(currentTrack->getMidiOutputDevice());
		applyAutoArm(r);
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
						tracks.at(i)->setSelected(true);
						applyAutoArm(tracks.at(i));
						propertyView->selectionChanged(tracks.at(i));

						for (int j = 0; j < tracks.at(i)->getRegions().size(); j++) {

							if (tracks.at(i)->getRegions().at(j)->getBounds().contains(x, y)) {
								tracks.at(i)->getRegions().at(j)->setSelected(true);
								propertyView->selectionChanged(tracks.at(i)->getRegions().at(j));
							}
							else if (!marqueeAdditive) {
								// Bei Shift/Cmd: bestehende Region-Selektion
								// nicht entwerten — Marquee will additiv sein.
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
