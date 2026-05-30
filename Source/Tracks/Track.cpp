/*
  ==============================================================================

	Track.cpp
	Created: 16 Dec 2016 9:40:38am
	Author:  pueskma

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "Track.h"
#include "TrackNavigator.h"
#include "../Session.h"
#include "../Mixer.h"
#include "../AudioUtils.h"
#include "MasterChannelPanel.h"
#include "../Actions/SplitRegionAction.h"
#include "../Project/Project.h"
#include <iostream>
#include <limits>
#include <cstdlib>
#include "../MessageBus/FastBus.h"
#include "AutomationLane.h"
#include "../PluginModule.h"

//==============================================================================
Track::Track(Type type, double sampleRate, MultiComponentDragger* dragger)
{
	this->constrainer = new ResizeConstrainer(this->zoom / 4);
	resizer = new ResizableEdgeComponent(this, constrainer, ResizableEdgeComponent::bottomEdge);
	resizer->setSize(getWidth(), 5);
	constrainer->setMaxWidth(1000);
	resizer->setTopLeftPosition(0, 70);
	// Alt-Drag-Sync: Track lauscht auf Resizer-Mouse-Events, damit wir in
	// mouseDrag() erkennen koennen, wann der Bottom-Edge mit gehaltenem Alt
	// gezogen wird und die Hoehe auf die Geschwistertracks gespiegelt werden
	// soll. wantsEventsForAllNestedChildComponents=false reicht, der Resizer
	// ist ein direktes Child von Track.
	resizer->addMouseListener(this, false);
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

	// Automation-Lane als Child anlegen (anfangs unsichtbar). Wird vom
	// TrackControl-Toggle ein-/ausgeblendet und verfolgt Track-Größe + Zoom.
	automationLane = std::make_unique<AutomationLane>(this, sampleRate);
	automationLane->setZoom(zoom);
	addChildComponent(automationLane.get());

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

    juce::Component::SafePointer<Track> safeThis(this);
    bus.subscribe<float>("tempo", [safeThis](const FastTopic<float>& topic) {
        if (auto* self = safeThis.getComponent()) {
            self->bpm = topic.getValue();
            self->repaint();
        }
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
	// Idempotent — bricht zugleich die Rueckkopplung Track <-> NameListener:
	// setName -> nameValue.setValue -> NameListener::valueChanged -> setName.
	if (this->name == name) return;
	Component::setName(name);
	this->name = name;
	// Den PropertyView-NameValue mitziehen, sonst zeigt das Property-Feld
	// nach einer Umbenennung aus dem Mixer (oder einem anderen Pfad) noch
	// den alten Wert, bis der User die Selection wechselt.
	if (nameValue != nullptr) nameValue->setValue(name);
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
			// Falls genau diese Region die laufende Aufnahme aufnimmt, den
			// per-Track-Pointer wegnehmen — sonst greift der naechste
			// TrackIn::eventReceived auf einen freed-pointer zu.
			clearRecordingRegionIfMatches(*it);
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

void Track::removeRegion(Region* r, bool clear) {
	if (r == nullptr) return;

	for (std::vector<Region*>::iterator it = regions.begin(); it != regions.end(); ++it) {
		if (*it != r) continue;

		clearRecordingRegionIfMatches(*it);
		if (dragger != nullptr)
			dragger->setSelected(*it, false);
		if (currentRegion == *it)
			currentRegion = nullptr;
		(*it)->removeAllChangeListeners();

		if (clear && audioBuffer != nullptr) {
			// Buffer-Bereich der Region nullen — aber hart clampen. Auto-
			// arrangierte Kopien koennen ueber das Track-Ende (audioBuffer-
			// Laenge) hinausragen; ein ungeclamptes clear() wuerde in JUCE
			// asserten/out-of-range gehen.
			const long off    = (long) (*it)->getSampleOffset();
			const long len    = (long) (*it)->getNumSamples();
			const long bufLen = (long) audioBuffer->getNumSamples();
			const long cs = juce::jlimit((long) 0, bufLen, off);
			const long ce = juce::jlimit((long) 0, bufLen, off + len);
			if (ce > cs)
				audioBuffer->clear((int) cs, (int) (ce - cs));
		}

		delete *it;
		regions.erase(it);
		repaint();
		return;
	}
}

AudioRegion* Track::mergeSelectedAudioRegions() {
	// Selektierte AudioRegions einsammeln (Region-Flag ODER Dragger,
	// gleiche Logik wie bounceSelectedRegionsInPlace).
	std::vector<AudioRegion*> selected;
	for (auto* r : regions) {
		auto* ar = dynamic_cast<AudioRegion*>(r);
		if (ar == nullptr) continue;
		const bool flagSel    = ar->isSelected();
		const bool draggerSel = (dragger != nullptr && dragger->isSelected(ar));
		if (flagSel || draggerSel)
			selected.push_back(ar);
	}
	if (selected.size() < 2) return nullptr;

	// Timeline-Range: minOffset .. maxEnd ueber alle selektierten Regionen.
	long minOffset = LONG_MAX;
	long maxEnd    = 0;
	for (auto* ar : selected) {
		const long off = ar->getSampleOffset();
		const long len = (long) ar->getNumSamples();
		if (off < minOffset) minOffset = off;
		if (off + len > maxEnd) maxEnd = off + len;
	}
	const long totalLen = maxEnd - minOffset;
	if (totalLen <= 0) return nullptr;

	// Merged-Buffer aufbauen. Stereo, additive Mix bei Overlap, Gaps bleiben
	// Stille (Buffer ist initial cleared).
	juce::AudioSampleBuffer merged(2, (int) totalLen);
	merged.clear();
	for (auto* ar : selected) {
		auto* src = ar->getBuffer();
		if (src == nullptr) continue;
		const long sss = ar->getSourceStartSample();
		const long len = (long) ar->getNumSamples();
		const long dstOff = ar->getSampleOffset() - minOffset;
		const long srcAvail = (long) src->getNumSamples() - sss;
		const long copyLen = juce::jmin(len, srcAvail);
		if (copyLen <= 0 || dstOff < 0) continue;

		const int srcChans = src->getNumChannels();
		for (int outCh = 0; outCh < 2; ++outCh) {
			const int srcCh = (srcChans == 1) ? 0 : juce::jmin(outCh, srcChans - 1);
			merged.addFrom(outCh, (int) dstOff, *src, srcCh, (int) sss, (int) copyLen);
		}
	}

	// Track-internen Mix-Buffer in den betroffenen Ranges leeren, sonst
	// bleibt der Originalbeitrag der entfernten Regionen drin und ueberlagert
	// spaeter die merged Region beim Playback.
	for (auto* ar : selected) {
		const long off = ar->getSampleOffset();
		const long len = (long) ar->getNumSamples();
		const long bufSamples = (long) audioBuffer->getNumSamples();
		const long clearStart = juce::jmax(0L, off);
		const long clearEnd   = juce::jmin(bufSamples, off + len);
		if (clearEnd > clearStart) {
			for (int ch = 0; ch < audioBuffer->getNumChannels(); ++ch)
				audioBuffer->clear(ch, (int) clearStart, (int) (clearEnd - clearStart));
		}
	}

	// Originals entfernen: aus dem regions-Vector ziehen, currentRegion ggf.
	// freigeben, dann delete (Component-Destruktor entfernt sich aus dem
	// Parent). Vor dem delete den eventuellen Recording-Pointer cleanen,
	// damit kein TrackIn::eventReceived in eine tote Region greift.
	for (auto* ar : selected) {
		clearRecordingRegionIfMatches(ar);
		ar->removeAllChangeListeners();
		auto it = std::find(regions.begin(), regions.end(), ar);
		if (it != regions.end())
			regions.erase(it);
		if (currentRegion == ar)
			currentRegion = nullptr;
		delete ar;
	}

	// Neue Region aus dem merged-Buffer anlegen. addRegion uebernimmt das
	// Layout-Setup, kopiert in den Track-internen audioBuffer, selektiert sie
	// und setzt currentRegion. Wir markieren sie als mutiert, damit der
	// naechste Project::save sie als WAV in $project/edits/ persistiert.
	AudioRegion* newRegion = addRegion(&merged, sampleRate, minOffset, totalLen);
	if (newRegion != nullptr) {
		newRegion->markBufferMutated();
		// Merge-Ergebnis ist per Definition konsolidiert (sss=0, visible == buffer)
		// — Indikator-Dreieck direkt setzen, damit der User es ohne erneutes
		// Consolidate sieht.
		newRegion->setConsolidated(true);
	}

	repaint();
	return newRegion;
}

MidiRegion* Track::mergeSelectedMidiRegions() {
	std::vector<MidiRegion*> selected;
	for (auto* r : regions) {
		auto* mr = dynamic_cast<MidiRegion*>(r);
		if (mr == nullptr) continue;
		const bool flagSel    = mr->isSelected();
		const bool draggerSel = (dragger != nullptr && dragger->isSelected(mr));
		if (flagSel || draggerSel)
			selected.push_back(mr);
	}
	if (selected.size() < 2) return nullptr;

	long minOffset = LONG_MAX;
	long maxEnd    = 0;
	for (auto* mr : selected) {
		const long off = mr->getSampleOffset();
		const long len = (long) mr->getNumSamples();
		if (off < minOffset) minOffset = off;
		if (off + len > maxEnd) maxEnd = off + len;
	}
	const long totalLen = maxEnd - minOffset;
	if (totalLen <= 0) return nullptr;

	// Neue leere MidiRegion an minOffset. addMidiRegion() pusht sie an die
	// regions-Liste und macht sie zur aktuellen Selektion.
	MidiRegion* mergedRegion = addMidiRegion(sampleRate, minOffset, totalLen);
	if (mergedRegion == nullptr) return nullptr;

	// Events aus den Source-Regionen einsammeln, in den merged-Buffer-Frame
	// umrechnen. Source-Visible-Range [sss, sss + visibleLen) -> merged-Range
	// [(srcOffset-minOffset), (srcOffset-minOffset)+visibleLen).
	for (auto* mr : selected) {
		if (mr == mergedRegion) continue; // safety, sollte nie passieren
		const long srcOffset  = mr->getSampleOffset();
		const long sss        = mr->getSourceStartSample();
		const long visibleLen = (long) mr->getNumSamples();
		for (auto* m : mr->getMessages()) {
			if (m == nullptr) continue;
			const long bufTs = (long) m->getTimeStamp();
			// Events ausserhalb der sichtbaren Range werden durch Trim
			// ausgeblendet und fliessen nicht in den Merge.
			if (bufTs < sss || bufTs >= sss + visibleLen) continue;
			const long regionRel = bufTs - sss;
			const long mergedTs  = (srcOffset - minOffset) + regionRel;
			mergedRegion->addMessageRaw(new juce::MidiMessage(*m), (juce::int64) mergedTs);
		}
	}

	mergedRegion->rebuildThumbnail();
	mergedRegion->updateThumb();

	// Originale entfernen — addMessageRaw hat oben Kopien gemacht, der
	// MidiRegion-Destruktor darf die Original-Messages also bedenkenlos
	// loeschen.
	for (auto* mr : selected) {
		if (mr == mergedRegion) continue;
		clearRecordingRegionIfMatches(mr);
		mr->removeAllChangeListeners();
		auto it = std::find(regions.begin(), regions.end(), mr);
		if (it != regions.end())
			regions.erase(it);
		if (currentRegion == mr)
			currentRegion = nullptr;
		delete mr;
	}

	currentRegion = mergedRegion;
	// Merge-Ergebnis ist per Definition konsolidiert — Indikator setzen.
	mergedRegion->setConsolidated(true);
	repaint();
	return mergedRegion;
}

bool Track::mergeSelectedRegions() {
	if (type == AUDIO)
		return mergeSelectedAudioRegions() != nullptr;
	if (type == MIDI || type == INSTRUMENT)
		return mergeSelectedMidiRegions() != nullptr;
	return false;
}

void Track::consolidateSelectedRegions() {
	// Selektion ueber beide Quellen pruefen (Region::selected ODER dragger) —
	// es gibt im Code Pfade die nur die eine oder andere setzen.
	for (auto* r : regions) {
		if (r == nullptr) continue;
		const bool sel = r->isSelected()
		              || (dragger != nullptr && dragger->isSelected(r));
		if (sel)
			r->consolidate();
	}
}

void Track::reverseSelectedAudioRegions() {
	for (auto* r : regions) {
		if (r == nullptr) continue;
		const bool sel = r->isSelected()
		              || (dragger != nullptr && dragger->isSelected(r));
		if (!sel) continue;
		if (auto* ar = dynamic_cast<AudioRegion*>(r)) {
			AudioUtils::reverseAudioBuffer(*ar->getBuffer());
			ar->markBufferMutated();
			ar->updateThumb();
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

		// Bei links-getrimmtem Original liegt der sichtbare Inhalt im Buffer
		// erst ab sourceStartSample. Wenn wir hier weiter mit startSample=0
		// kopieren, bekommt das Duplikat die abgeschnittene Front statt der
		// sichtbaren Region — und Track::getSample (das jetzt mit
		// inSource=sample-offset+sourceStartSample liest) findet im
		// Duplikat-Buffer auch nichts Sinnvolles. Daher den Source-Offset
		// uebernehmen; das Duplikat selbst bleibt konsolidiert (sss=0).
		const long srcStart = (long) _region->getSourceStartSample();

		// Sample-akkurate Laenge: NICHT getNumSamples() — das geht im
		// "reduced"-Pfad ueber `(getWidth()/zoom)*sampleRate` (int-Trunkierung)
		// und verliert Samples, sobald die Pixel-Breite der Region nicht exakt
		// zum Sample-Count passt. Genau das tritt bei frisch konsolidierten
		// Regionen mit nicht-rationaler Quarter-Note-Sample-Laenge auf
		// (typisch jedes Tempo ausser 60/120/240... bei 44.1/48 kHz): width
		// und thumbnailBounds.width laufen je nach Codepfad einmal ueber round,
		// einmal ueber trunc und unterscheiden sich um 1 Pixel — daraus wird in
		// getNumSamples ein um bis zu samples-per-pixel zu kleiner Wert.
		// trimmedNumSamples wird von jedem Ctor (Slice/Buffer/File/Record)
		// sample-genau gesetzt und ist die richtige Quelle.
		const long fullBuffer = (long) _region->getBuffer()->getNumSamples();
		long numSamplesToCopy = _region->getTrimmedNumSamples();
		if (numSamplesToCopy <= 0)
			numSamplesToCopy = juce::jmax(0L, fullBuffer - srcStart);
		// Nie ueber das Buffer-Ende hinaus kopieren — der slice-Ctor wuerde
		// sonst beim copyFrom in JUCE asserten.
		numSamplesToCopy = juce::jmin(numSamplesToCopy, juce::jmax(0L, fullBuffer - srcStart));

		AudioRegion* duplicate = new AudioRegion(_region, *manager, sampleRate, srcStart, numSamplesToCopy);
		duplicate->setDragger(dragger);

		// Sample-akkurate Platzierung: das Duplikat steht exakt einen Region-
		// Block nach dem Source. Frueher lief der Offset ueber Pixel:
		//   newX            = getX() + getWidth()      // pixel-int Summe
		//   newSampleOffset = (long)((newX/zoom)*sr)   // (long) trunkiert
		// Beide Schritte verlieren bis zu ein paar Samples (int-Pixel-Rundung
		// + (long)-Trunkierung), nach ein paar Duplikaten ist die Region nicht
		// mehr auf dem Takt. Direkt im Sample-Raum bleiben — kein Roundtrip.
		long newSampleOffset = _region->getSampleOffset() + numSamplesToCopy;

		// Falls die Stelle bereits belegt ist, an die nächste freie Position rücken.
		newSampleOffset = findNextFreeOffset(nullptr, newSampleOffset, numSamplesToCopy);

		// Bounds aus exakten Samples mit konsistentem Runden (round, nicht
		// trunc) — so faellt die Pixel-Breite des Duplikats deterministisch
		// gleich gross aus wie die jedes anderen Duplikats derselben Source.
		const int newX = (int) std::round((double) newSampleOffset / sampleRate * zoom);
		const int newW = (int) std::round((double) numSamplesToCopy / sampleRate * zoom);

		// WICHTIG: Setze Bounds NACHDEM der Konstruktor die initiale Größe gesetzt hat.
		// Flag setzen, damit der componentMovedOrResized-Handler dieses setBounds
		// nicht als User-Resize interpretiert (er wuerde sonst sourceStartSample
		// auf newSampleOffset hochsetzen, der Lookup im Track::getSample landete
		// dann hinter dem Buffer-Inhalt und die Region waere stumm).
		Rectangle<int>* bounds = new Rectangle<int>(0, 0, newW, getHeight());
		duplicate->setBoundsUpdateInProgress(true);
		duplicate->setBounds(newX, 0, newW, region->getHeight());
		duplicate->setBoundsUpdateInProgress(false);
		duplicate->setThumbnailBounds(bounds);
		duplicate->setLoopCount(0);

		this->regions.push_back(duplicate);
		this->currentRegion = duplicate;

		this->currentRegion->toFront(true);
		addAndMakeVisible(duplicate);

		clearSelection();
		dragger->setSelected(duplicate, true);

		// quantize=false: das default-true im Region::setSampleOffset klemmt
		// den Offset auf bufferSize-Vielfache ab (z.B. /512 *512). Genau das
		// macht die "auf den Takt eingerastet"-Erwartung kaputt — wir haben den
		// Offset oben sample-akkurat berechnet, kein Grund ihn wieder zu
		// runden.
		duplicate->setSampleOffset(newSampleOffset, false, false, false);
		duplicate->setOffset(newX);
		duplicate->setClipRefId(_region->getClipRefid());
		duplicate->addChangeListener(this);

		// WICHTIG: setZoom NACH setSampleOffset aufrufen
		// Dadurch wird die Position korrekt neu berechnet
		if (zoom > 0)
			duplicate->setZoom(zoom);

		// setZoom ruft intern snapToQuarterNote(sampleOffset) — fuer Tempi,
		// deren Viertelnote keine ganzzahlige Sample-Laenge hat (z.B. 110 BPM
		// bei 44.1 kHz), schiebt das den Offset minimal weg, *und* setZoom
		// schreibt die snappte Position zurueck. Fuer ein programmatisch
		// platziertes Duplikat ist das ungewollt — Original-Position und
		// -Breite hier wieder explizit setzen.
		duplicate->setBoundsUpdateInProgress(true);
		duplicate->setBounds(newX, 0, newW, region->getHeight());
		duplicate->setBoundsUpdateInProgress(false);
		duplicate->setSampleOffset(newSampleOffset, false, false, false);
		duplicate->setOffset(newX);

		// Kopiere nur die tatsächlich verwendeten Samples in den Track-Buffer
		// (Magnitude-Anzeige). audioBuffer kann kuerzer sein, wenn die Region
		// ueber das Track-Ende hinausragt — clampen.
		if (audioBuffer != nullptr && duplicate->getBuffer() != nullptr) {
			const long copyLen = juce::jmin((long) numSamplesToCopy,
			                                (long) audioBuffer->getNumSamples() - newSampleOffset,
			                                (long) duplicate->getBuffer()->getNumSamples());
			if (copyLen > 0) {
				if (duplicate->getBuffer()->getNumChannels() == 1) {
					audioBuffer->copyFrom(0, newSampleOffset, *duplicate->getBuffer(), 0, 0, (int) copyLen);
					audioBuffer->copyFrom(1, newSampleOffset, *duplicate->getBuffer(), 0, 0, (int) copyLen);
				}
				else {
					audioBuffer->copyFrom(0, newSampleOffset, *duplicate->getBuffer(), 0, 0, (int) copyLen);
					audioBuffer->copyFrom(1, newSampleOffset, *duplicate->getBuffer(), 1, 0, (int) copyLen);
				}
			}
		}

		repaint();
	}
	else if (MidiRegion* mregion = dynamic_cast<MidiRegion*>(region)) {
		const long numSamples = (long) mregion->getNumSamples();
		long newOffset = mregion->getSampleOffset() + numSamples;
		// Falls die Stelle bereits belegt ist, an die nächste freie Position rücken.
		newOffset = findNextFreeOffset(nullptr, newOffset, numSamples);

		// Bounds aus exakten Samples mit konsistentem Runden — analog zum
		// AudioRegion-Pfad. Vorher: newX per (long)-Trunkierung, Width per
		// source->getWidth() (= source's gerundete Pixel-Breite, kann um 1 px
		// von round(numSamples/sr*zoom) abweichen) → das Duplikat lag fuer
		// nicht-rationale Tempi nicht exakt am Source-Ende.
		const int newX = (int) std::round((double) newOffset  / sampleRate * zoom);
		const int newW = (int) std::round((double) numSamples / sampleRate * zoom);

		MidiRegion* clone = mregion->clone(newOffset);
		clone->setDragger(dragger);
		// Resize-Handler beim initialen Bounds-Set ausschalten — gleiche
		// Logik wie im AudioRegion-Pfad: das setBounds aendert X und Width
		// gleichzeitig, der combined-resize-Branch in MidiRegion wuerde das
		// sonst als User-Trim verstehen und sourceStartSample falsch setzen.
		clone->setBoundsUpdateInProgress(true);
		clone->setBounds(newX, 0, newW, getHeight());
		clone->setBoundsUpdateInProgress(false);
		// MidiRegion::clone() ruft intern setZoom() + setNumSamples() — beide
		// stempeln nach unserem setBounds nochmal an die clone-eigene
		// sampleOffset/numSamples-Math. Den raw sampleOffset und die Pixel-
		// Bounds danach explizit wieder auf unsere sample-genauen Werte
		// setzen, damit der Bar-Snap ueberlebt.
		clone->setSampleOffset(newOffset, false, false, false);
		clone->setBoundsUpdateInProgress(true);
		clone->setBounds(newX, 0, newW, getHeight());
		clone->setBoundsUpdateInProgress(false);
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

		// Cross-Track-Paste: a gehoert zu einer ANDEREN Track-Instanz mit
		// eigenem audioBuffer. Der Slice-Ctor liest aus a->getBuffer() ueber
		// srcStart..srcStart+numSamples — der Bereich MUSS hart geclamped
		// werden, sonst asserted/crashed JUCE in AudioSampleBuffer::copyFrom.
		// duplicateRegion (cmd+D) macht genau dieses Clamp; pasteRegion (cmd+V)
		// hat es vergessen und sich auf getNumSamples() verlassen, das je nach
		// Trim-Pfad bis zu samples-per-pixel zu klein/zu gross zurueckkommt.
		auto* srcBuf = a->getBuffer();
		if (srcBuf == nullptr || srcBuf->getNumSamples() <= 0)
			return nullptr;

		const long fullBuffer = (long) srcBuf->getNumSamples();
		const long srcStart   = juce::jlimit(0L, fullBuffer, (long) a->getSourceStartSample());

		long numSamplesToCopy = a->getTrimmedNumSamples();
		if (numSamplesToCopy <= 0)
			numSamplesToCopy = juce::jmax(0L, fullBuffer - srcStart);
		numSamplesToCopy = juce::jmin(numSamplesToCopy, juce::jmax(0L, fullBuffer - srcStart));
		if (numSamplesToCopy <= 0)
			return nullptr;

		AudioRegion* duplicate = new AudioRegion(a, *manager, sampleRate, srcStart, numSamplesToCopy);
		duplicate->setDragger(dragger);

		long newSampleOffset = findNextFreeOffset(nullptr, targetSampleOffset, numSamplesToCopy);
		// Sample-akkurate Pixel-Werte mit konsistentem Runden — analog zu
		// duplicateRegion. (int)-Cast trunkiert und schiebt das Duplikat
		// systematisch um bis zu 1 Pixel nach links.
		const int newX = (int) std::round((double) newSampleOffset / sampleRate * zoom);
		const int newW = (int) std::round((double) numSamplesToCopy / sampleRate * zoom);

		Rectangle<int>* bounds = new Rectangle<int>(0, 0, newW, getHeight());
		duplicate->setBoundsUpdateInProgress(true);
		duplicate->setBounds(newX, 0, newW, getHeight());
		duplicate->setBoundsUpdateInProgress(false);
		duplicate->setThumbnailBounds(bounds);
		duplicate->setLoopCount(0);

		this->regions.push_back(duplicate);
		this->currentRegion = duplicate;
		addAndMakeVisible(duplicate);
		duplicate->toFront(true);

		clearSelection();
		dragger->setSelected(duplicate, true);

		// quantize=false: das default-true in Region::setSampleOffset rastet
		// den Offset auf bufferSize-Vielfache — der hier berechnete
		// newSampleOffset ist sample-genau, jede Quantisierung verschiebt das
		// Duplikat und macht die untenstehende Mirror-Kopie inkonsistent zur
		// Region-Position.
		duplicate->setSampleOffset(newSampleOffset, false, false, false);
		duplicate->setOffset(newX);
		duplicate->setClipRefId(a->getClipRefid());
		duplicate->addChangeListener(this);

		if (zoom > 0)
			duplicate->setZoom(zoom);

		// setZoom ruft intern snapToQuarterNote(sampleOffset) — siehe Kommentar
		// in duplicateRegion. Position danach explizit wiederherstellen.
		duplicate->setBoundsUpdateInProgress(true);
		duplicate->setBounds(newX, 0, newW, getHeight());
		duplicate->setBoundsUpdateInProgress(false);
		duplicate->setSampleOffset(newSampleOffset, false, false, false);
		duplicate->setOffset(newX);

		// Mirror the duplicate's audio into the track's shared audioBuffer
		// at the new offset (Magnitude). Mit Out-of-bounds-Schutz.
		if (audioBuffer != nullptr && duplicate->getBuffer() != nullptr) {
			const long copyLen = juce::jmin((long) numSamplesToCopy,
			                                (long) audioBuffer->getNumSamples() - newSampleOffset,
			                                (long) duplicate->getBuffer()->getNumSamples());
			if (copyLen > 0) {
				if (duplicate->getBuffer()->getNumChannels() == 1) {
					audioBuffer->copyFrom(0, newSampleOffset, *duplicate->getBuffer(), 0, 0, (int) copyLen);
					audioBuffer->copyFrom(1, newSampleOffset, *duplicate->getBuffer(), 0, 0, (int) copyLen);
				}
				else {
					audioBuffer->copyFrom(0, newSampleOffset, *duplicate->getBuffer(), 0, 0, (int) copyLen);
					audioBuffer->copyFrom(1, newSampleOffset, *duplicate->getBuffer(), 1, 0, (int) copyLen);
				}
			}
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
		clone->setBoundsUpdateInProgress(true);
		clone->setBounds(newX, 0, source->getWidth(), getHeight());
		clone->setBoundsUpdateInProgress(false);
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

	// Mono-Quellen liefern nur Kanal 0 — direkt aus Kanal 1 zu lesen waere ein
	// Out-of-Range-Zugriff. Mono auf beide Bus-Kanaele spiegeln (wie im
	// positionslosen addRegion-Overload).
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

juce::AudioSampleBuffer* Track::bounceRegionToBuffer(Region* sourceRegion)
{
	if (sourceRegion == nullptr) return nullptr;
	auto* mr = dynamic_cast<MidiRegion*>(sourceRegion);
	if (mr == nullptr) return nullptr; // erste Version: nur MIDI -> Audio
	if (pluginModule == nullptr || pluginModule->getPlugin() == nullptr) return nullptr;

	auto* instrument = pluginModule->getPlugin();
	const int sr        = (int) Project::getInstance()->getSampleRate();
	const int blockSize = juce::jmax(64, Project::getInstance()->getBufferSize());
	const long lenSamples = mr->getNumSamples();
	if (lenSamples <= 0 || sr <= 0) return nullptr;

	// Chain = Instrument + alle belegten Channel-Insert-FX in Slot-Reihenfolge.
	// Diese Reihenfolge spiegelt den Realtime-Pfad (rewireChain: source ->
	// fxChain[0] -> ... -> AudioOut), damit der Bounce klingt wie live.
	struct ChainSlot {
		PluginModule*              pm;
		juce::AudioPluginInstance* p;
	};
	std::vector<ChainSlot> chain;
	chain.push_back({ pluginModule, instrument });
	for (int i = 0; i < NUM_FX_SLOTS; ++i) {
		PluginModule* fx = fxPlugins[i];
		if (fx == nullptr) continue;
		auto* fxp = fx->getPlugin();
		if (fxp == nullptr) continue;
		chain.push_back({ fx, fxp });
	}

	// Summierte Plugin-Latenz der gesamten Kette. Plugins melden hier die
	// Verzoegerung, die ihr Output gegenueber dem Input hat (z.B. Lookahead-
	// Limiter, FFT-basierte EQs, Convolution-Reverbs). Ohne Kompensation
	// rutscht das Bounce-Audio um diesen Betrag *nach hinten* relativ zur
	// MIDI-Spur, weil wir die ersten N Samples am Ausgang noch aus dem leeren
	// Pre-Roll bekommen. Wir rendern stattdessen lenSamples + totalLatency und
	// werfen die fuehrenden totalLatency Samples weg.
	int totalLatency = 0;
	for (auto& s : chain)
		totalLatency += juce::jmax(0, s.p->getLatencySamples());

	const int renderLen = (int) lenSamples + totalLatency;

	auto* outBuffer = new juce::AudioSampleBuffer(2, (int) lenSamples);
	outBuffer->clear();

	// MIDI-Events einsammeln und auf bounce-relative Sampletimes mappen
	// (0 == linke Kante der sichtbaren Region). sourceStartSample abziehen,
	// damit links-getrimmte Regionen nicht den Pre-Trim-Bereich rendern.
	juce::MidiBuffer regionMidi;
	const long sss = mr->getSourceStartSample();
	for (int k = 0; k < mr->getNumMessages(); ++k)
	{
		auto* msg = mr->getMessageAt(k);
		if (msg == nullptr) continue;
		const int t = (int) msg->getTimeStamp() - (int) sss;
		if (t < 0 || t >= (int) lenSamples) continue;
		regionMidi.addEvent(*msg, t);
	}

	// Plugins offline vorbereiten + RAII-Reset fuer die gesamte Kette.
	struct ChainGuard {
		std::vector<ChainSlot>* c;
		~ChainGuard() {
			if (c == nullptr) return;
			for (auto& s : *c) {
				if (s.p)  s.p->setNonRealtime(false);
				if (s.pm) s.pm->setBounceMode(false);
			}
		}
	} guard { &chain };

	for (auto& s : chain) {
		s.p->setNonRealtime(true);
		s.pm->setBounceMode(true);
		s.p->prepareToPlay((double) sr, blockSize);
		// reset() loescht Filter-/Delay-/Reverb-State von einer evtl. vorher
		// laufenden Realtime-Wiedergabe — ohne das wuerde der Bounce mit dem
		// Tail des letzten Live-Spielens beginnen.
		s.p->reset();
	}

	// Eine gemeinsame Block-Buffer-Groesse fuer die ganze Kette: gross genug
	// fuer den breitesten Plugin-IO der Kette, mindestens stereo.
	int maxChs = 2;
	for (auto& s : chain)
		maxChs = juce::jmax(maxChs, s.p->getTotalNumInputChannels(), s.p->getTotalNumOutputChannels());

	juce::AudioSampleBuffer blockBuf(maxChs, blockSize);
	juce::MidiBuffer emptyMidi;

	for (int pos = 0; pos < renderLen; pos += blockSize)
	{
		const int n = juce::jmin(blockSize, renderLen - pos);

		// MIDI nur in den ersten lenSamples einspeisen — der totalLatency-Tail
		// danach ist reines "Plugin austrudeln lassen", keine neuen Noten.
		juce::MidiBuffer slice;
		if (pos < (int) lenSamples) {
			for (const auto md : regionMidi)
			{
				const int sp = md.samplePosition;
				if (sp >= pos && sp < pos + n)
					slice.addEvent(md.getMessage(), sp - pos);
			}
		}

		// processBlock liest/schreibt in-place. Vor dem Instrument wird der
		// Buffer geleert (kein Audio-Input ins Instrument), danach laeuft der
		// Kette-Ausgang Plugin-zu-Plugin durch denselben Buffer.
		blockBuf.clear();
		for (size_t i = 0; i < chain.size(); ++i)
		{
			auto* p = chain[i].p;
			juce::MidiBuffer& mb = (i == 0) ? slice : emptyMidi;

			if (n != blockSize) {
				// JUCE processBlock verwendet immer buffer.getNumSamples() —
				// fuer den letzten Teilblock auf n shrinken und danach wieder
				// auf blockSize zurueckdrehen, damit Allokationen vermieden
				// werden.
				blockBuf.setSize(blockBuf.getNumChannels(), n, true, false, true);
				p->processBlock(blockBuf, mb);
				blockBuf.setSize(blockBuf.getNumChannels(), blockSize, true, false, true);
			} else {
				p->processBlock(blockBuf, mb);
			}
		}

		// Latenzkompensiert in outBuffer schreiben: erst ab Sample-Index
		// totalLatency im Render-Stream beginnt das, was zeitlich zu Sample 0
		// der MIDI-Region gehoert. Vorhergehende Samples enthalten den
		// Plugin-Pre-Roll und werden verworfen.
		const int srcStart   = juce::jmax(0, totalLatency - pos); // 0 sobald wir hinter dem Pre-Roll sind
		const int dstStart   = juce::jmax(0, pos - totalLatency);
		const int copyAvail  = n - srcStart;
		const int copyRoom   = (int) lenSamples - dstStart;
		const int copyLen    = juce::jmin(copyAvail, copyRoom);
		if (copyLen <= 0) continue;

		const int chs = juce::jmin(2, blockBuf.getNumChannels());
		for (int ch = 0; ch < chs; ++ch)
			outBuffer->copyFrom(ch, dstStart, blockBuf, ch, srcStart, copyLen);
		if (chs == 1)
			outBuffer->copyFrom(1, dstStart, blockBuf, 0, srcStart, copyLen);
	}
	return outBuffer;
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
    if (!baseRegion)
        return;

    // Nur selektierte Regionen am Marker schneiden. Ohne diese Pruefung wuerde
    // der von TrackNavigator ueber alle Tracks ausgerollte Split-Loop jede
    // Region unter dem Playhead schneiden — auch wenn der User nur eine
    // bestimmte gemeint hat. Selektion kann ueber zwei Quellen kommen:
    // Region::selected (Flag, gesetzt z.B. via TrackNavigator-Hintergrundklick)
    // oder MultiComponentDragger (Direkt-Klick auf die Region selbst).
    const bool isSelected = baseRegion->isSelected()
                         || (dragger != nullptr && dragger->isSelected(baseRegion));
    if (!isSelected)
        return;

    // Edge-Case: Marker liegt genau am Rand der Region — kein sinnvoller Split.
    const long offset = baseRegion->getSampleOffset();
    const long len    = baseRegion->getNumSamples();
    const long numLeft  = sampleNum - offset;
    const long numRight = len - numLeft;
    if (numLeft <= 0 || numRight <= 0)
        return;

    // Pixel-Geometrie: rechte Region beginnt direkt nach der linken (also
    // baseRegion.X + Pixel-Breite-der-linken-Haelfte). Vorher wurde rightX
    // aus sampleNum/sampleRate*zoom berechnet — das ignoriert aber dass die
    // baseRegion selbst nicht am Track-Anfang (X=0) liegen muss. Resultat:
    // gap zwischen den Haelften und u.U. landet die rechte Region soweit
    // links dass sie von clampOffsetForRegion weggeschoben oder visuell
    // unsichtbar wird.
    const int leftX  = baseRegion->getX();
    const int leftW  = (int) std::round((double) numLeft  / sampleRate * zoom);
    const int rightX = leftX + leftW;
    const int rightW = (int) std::round((double) numRight / sampleRate * zoom);

    if (AudioRegion* region = dynamic_cast<AudioRegion*>(baseRegion)) {
        // Buffer-absolute Offsets uebergeben (der Slice-Ctor liest direkt aus
        // other->getBuffer()): wenn die Quelle bereits links getrimmt war
        // (sourceStartSample > 0), faengt der sichtbare Inhalt erst dort an.
        // Mit region-relativen Offsets (0 / numLeft) wuerden wir sonst die
        // abgeschnittene Front in die linke Haelfte kopieren.
        const long otherSss = region->getSourceStartSample();
        AudioRegion* leftRegion  = new AudioRegion(region, *manager, sampleRate, otherSss,           numLeft);
        AudioRegion* rightRegion = new AudioRegion(region, *manager, sampleRate, otherSss + numLeft, numRight);

        Rectangle<int>* leftBounds  = new Rectangle<int>(0, 0, leftRegion->getThumbnail()->getTotalLength() * 20, getHeight());
        Rectangle<int>* rightBounds = new Rectangle<int>(0, 0, rightRegion->getThumbnail()->getTotalLength() * 20, getHeight());

        // Component-Listener temporaer abklemmen, sonst rechnet
        // AudioRegion::componentMovedOrResized nach unserem setBounds einen
        // Pixel->Sample->Pixel-Roundtrip (plus optional Snap-to-Grid). Der
        // verschiebt die Region um 1 Pixel und produziert sichtbare Gaps
        // zwischen den Split-Haelften. Auch sourceStartSample wuerde dort
        // umgerechnet werden — das Vorab-setzen von sampleOffset reicht
        // nicht aus, weil der Handler trotzdem newX/newWidth neu setzt.
        leftRegion->removeComponentListener(leftRegion);
        rightRegion->removeComponentListener(rightRegion);

        leftRegion->setSampleOffset(offset, false, false);
        leftRegion->setOffset(region->getOffset());
        leftRegion->setBounds(leftX, 0, leftW, getHeight());
        leftRegion->setThumbnailBounds(leftBounds);

        rightRegion->setSampleOffset(sampleNum, false, false);
        rightRegion->setOffset((double)sampleNum / sampleRate);
        rightRegion->setBounds(rightX, 0, rightW, getHeight());
        rightRegion->setThumbnailBounds(rightBounds);

        leftRegion->addComponentListener(leftRegion);
        rightRegion->addComponentListener(rightRegion);

        Project::getInstance()->getUndoManager()->beginNewTransaction();
        Project::getInstance()->getUndoManager()->perform(
            new SplitRegionAction(this, region, leftRegion, rightRegion));

        // attachRegion() (gerade von SplitRegionAction::perform ausgefuehrt)
        // ruft AudioRegion::setZoom auf, das unbedingt sampleOffset auf die
        // naechste Viertelnote snappt. Fuer den Split ist das ungewollt: die
        // rechte Haelfte beginnt am Playhead, der nur zufaellig auf einem
        // Viertel liegt — sonst entsteht eine sichtbare Luecke zwischen den
        // Haelften, und die rechte rastet zum Raster. Workaround analog
        // duplicateRegion: Position und Offset nach setZoom explizit
        // wiederherstellen. setBoundsUpdateInProgress(true) blockt den
        // componentMovedOrResized-Reentrant, sonst snappt der gleich nochmal.
        auto restoreSplitHalf = [this](AudioRegion* half, long sampleOff, int x, int w) {
            half->setBoundsUpdateInProgress(true);
            half->setBounds(x, 0, w, getHeight());
            half->setBoundsUpdateInProgress(false);
            half->setSampleOffset(sampleOff, false, false, false);
            half->setOffset(x);
        };
        restoreSplitHalf(leftRegion,  offset,    leftX,  leftW);
        restoreSplitHalf(rightRegion, sampleNum, rightX, rightW);
        return;
    }

    if (MidiRegion* region = dynamic_cast<MidiRegion*>(baseRegion)) {
        auto [leftRegion, rightRegion] = region->splitAt(sampleNum, dragger);
        if (!leftRegion || !rightRegion)
            return;

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
	// Lane-Automation: bei jedem Block den interpolierten Wert puffern, damit
	// der Message-Thread (Track::applyPendingAutomation, getriggert vom
	// TrackNavigator-Timer) ihn ans Plugin durchreicht.
	//
	// Wir rufen hier explizit *nicht* setValue/setValueNotifyingHost am
	// Plugin-Parameter — VST3 setParamNormalized ist per Spec message-thread,
	// und viele Plugins triggern aus dem setValue-Pfad synchron ihre UI
	// (z.B. MainUI::parameterChanged), was vom Audio-Thread aus crasht,
	// sobald das Plugin-Fenster offen ist.
	if (dispatchEvents && !cachedAutomationParams.empty())
	{
		for (int li = 0; li < automationModel.numLanes(); ++li)
		{
			auto* lane = automationModel.laneAt(li);
			if (lane == nullptr || lane->isEmpty()) continue;
			const juce::String& laneId = lane->getParameterId();
			for (auto& entry : cachedAutomationParams)
			{
				if (entry == nullptr) continue;
				if (entry->scopedId == laneId)
				{
					const float v = juce::jlimit(0.0f, 1.0f, lane->valueAt(position));
					entry->pendingValue.store(v, std::memory_order_relaxed);
					entry->hasPending.store(true, std::memory_order_release);
					break;
				}
			}
		}
	}

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

		// Mute / Solo: stumm geschaltete bzw. nicht-solo'te MIDI-/Instrument-
		// Tracks duerfen keine Events mehr an Plugins/MIDI-Out feuern. Vorher
		// noch alle aktiven Noten beenden, sonst bleiben sie haengen, sobald
		// jemand Mute/Solo waehrend des Spielens umlegt.
		const bool anySolo = Mixer::getInstance()->anyTrackSoloed();
		if (isMute() || (anySolo && !isSolo())) {
			if (!activeNoteNumbers.empty())
				sendAllNotesOff();
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
					// Volle bufferSize aus nextR dispatchen, NICHT nur
					// (blockEnd - rEnd). MidiRegion::sampleOffset wird vom
					// ctor / componentMovedOrResized auf bufferSize-Vielfache
					// abgerundet — wenn A.numSamples kein Vielfaches von bs ist
					// (typisch: 1 Takt @ 120 BPM/44.1 kHz = 88200 != n*512),
					// liegt B.sampleOffset um K = A.end - B.sampleOffset Samples
					// VOR A.end. Lead-Len waere dann nur bs - K, und der naechste
					// Block fragt B ab sampleNum = bufferSize → die K Samples
					// in B's Buffer-Range [bs-K, bs) werden nie ausgeliefert.
					// Mit voller bs hier deckt das Lead [0, bs) ab, der Folge-
					// block [bs, 2bs) → keine Luecke an der Boundary.
					dispatchMidiMessages(nextR->getMessages(0, bufferSize));
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

	if (automationLane != nullptr)
		automationLane->setZoom(zoom);
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
		// Region-relative position innerhalb der Quell-Daten ableiten.
		// sourceStartSample > 0 nach Resize-vorne: der Lookup verschiebt sich
		// um diesen Betrag in den Buffer hinein. Konsolidierte Region: sss=0.
		const long inSource = (sample - currentRegion->getSampleOffset())
		                    + currentRegion->getSourceStartSample();
		auto* buf = getBuffer();
		if (buf != nullptr && inSource >= 0 && inSource < buf->getNumSamples()) {
			// Per-region gain via virtual base call. Region default is 1.0,
			// AudioRegion overrides; MIDI/instrument regions still return 1.0.
			// Multiplying unconditionally avoids a branch in the audio loop.
			const float g = currentRegion->getGain();
			if (buf->getNumChannels() == 1) {
				return buf->getSample(0, inSource) * g;
			}
			else {
				return buf->getSample(channel, inSource) * g;
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
	// Defensive: bei unbekannten Kanal-Indizes (negativ, > 1) liefern wir 0
	// statt aus Versehen magnitudeRight zurueckzugeben — das hat in Meter-
	// Anzeigen frueher zu vertauschten Pegeln gefuehrt, sobald jemand
	// channel=2 fuer 5.1 o.ae. uebergibt.
	if (channel == 0) return this->magnitudeLeft;
	if (channel == 1) return this->magnitudeRight;
	return 0.0;
}

void Track::updateMagnitude(int sample, int buffersize, float gainLeft, float gainRight) {
	// Robustness gegen kaputte Aufrufe vom Audio-Thread: TrackIn::process
	// laeuft im Realtime-Callback, und audioBuffer kann waehrend Open/Clear/
	// Buffer-Resize voruebergehend null oder leer sein. JUCE's
	// AudioBuffer::getMagnitude jasserted bei channel/range-Verletzungen —
	// in Release wuerde es OOB lesen.
	if (audioBuffer == nullptr) {
		this->magnitudeLeft = 0.0;
		this->magnitudeRight = 0.0;
		return;
	}

	const int numCh      = audioBuffer->getNumChannels();
	const int numSamples = audioBuffer->getNumSamples();

	if (numCh <= 0 || numSamples <= 0 || buffersize <= 0) {
		this->magnitudeLeft = 0.0;
		this->magnitudeRight = 0.0;
		return;
	}

	// Range in den Buffer schieben, dann Laenge clampen. Negative sample-
	// Werte (z.B. wenn samplePos beim Reset kurzzeitig < 0 ist) sonst direkt
	// in jassert.
	int start = juce::jmax (0, sample);
	int len   = juce::jmin (buffersize, numSamples - start);

	if (start >= numSamples || len <= 0) {
		this->magnitudeLeft = 0.0;
		this->magnitudeRight = 0.0;
		return;
	}

	this->magnitudeLeft  = (numCh > 0)
		? audioBuffer->getMagnitude (0, start, len) * gainLeft
		: 0.0;
	this->magnitudeRight = (numCh > 1)
		? audioBuffer->getMagnitude (1, start, len) * gainRight
		: this->magnitudeLeft;   // Mono-Buffer: rechts spiegelt links
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

juce::Colour Track::getTrackColour() const {
	// Hue aus der Position im Navigator. Goldener-Schnitt-Schritt (0.61803…)
	// gibt auch bei wenigen Tracks eine gut verteilte Palette ohne harte
	// Wiederholungen — gleichzeitig deterministisch, also bleibt die Farbe
	// stabil solange der Index gleich bleibt.
	int index = 0;
	if (navigator != nullptr) {
		const auto& tracks = const_cast<TrackNavigator*>(navigator)->getTracks();
		for (size_t i = 0; i < tracks.size(); ++i) {
			if (tracks[i] == this) { index = (int)i; break; }
		}
	}

	const float hue = std::fmod(0.05f + index * 0.61803398875f, 1.0f);
	// Gedämpfte Saturation/Brightness — knallige Farben würden auf dem
	// dunklen Editor-Hintergrund die Wellenform/Notes überstrahlen.
	return juce::Colour::fromHSV(hue, 0.55f, 0.55f, 1.0f);
}

void Track::paint(Graphics& g) {
	const juce::Colour base = getTrackColour();

	// Track-Hintergrund: deutlich abgedunkelter Tint der Track-Farbe, damit
	// der Bezug zur Region erkennbar bleibt aber nicht blendet.
	const juce::Colour bg = selected
		? base.withMultipliedSaturation(0.55f).withMultipliedBrightness(0.55f)
		: base.withMultipliedSaturation(0.40f).withMultipliedBrightness(0.32f);

	g.fillAll(bg);

	// Linker farbiger Streifen als „Track-Identifier" (wie ein Tab-Header).
	g.setColour(base);
	g.fillRect(0, 0, 3, getHeight());

	g.setColour(Colours::black.withAlpha(0.4f));
	g.drawLine(0, (float)getHeight(), (float)getWidth(), (float)getHeight());
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

	// Bar-1-Strich (= Beschriftung "1" auf der Timeline, sample == 1 Takt
	// Laenge) deutlich hervorheben — analog zur Timeline.
	const double samplesPerBeat = sampleRate / (bpm / 60.0);
	const long   samplesPerBar  = (long) std::round(samplesPerBeat * 4.0);

	// WICHTIG: Verwende exakte Multiplikation statt akkumulierte Addition
	int gridCount = (int)(getWidth() * sampleRate / (zoom * samplesPerGrid)) + 2;

	// Dynamische Rasterdichte analog zur Timeline: weit rausgezoomt wuerden die
	// Gridlinien sonst zu einem grauen Block verschmelzen. gridStep ist die
	// kleinste Zweierpotenz, bei der die gezeichneten Linien mindestens
	// minGridSpacingPx auseinanderliegen — der Bar-1-Strich wird unabhaengig
	// davon immer gezeichnet.
	const double pixelsPerGrid   = (double) samplesPerGrid * zoom / sampleRate;
	const double minGridSpacingPx = 28.0;
	int gridStep = 1;
	if (pixelsPerGrid > 0.0)
		while (pixelsPerGrid * gridStep < minGridSpacingPx)
			gridStep *= 2;

	for (int i = 0; i <= gridCount; i++) {
		// Exakte Berechnung für jeden Grid-Punkt
		long exactSample = i * samplesPerGrid;
		double exactPixel = (double)exactSample * zoom / sampleRate;
		int pixelPos = (int)std::round(exactPixel);

		const bool isBarOne = (exactSample == samplesPerBar);
		if ((i % gridStep) != 0 && !isBarOne)
			continue;

		if (pixelPos >= 0 && pixelPos < getWidth()) {
			if (isBarOne) {
				// Dicker, deutlich sichtbarer Strich am Ende von Takt 0 /
				// Beginn von Takt 1.
				g.setColour(Colours::white.withAlpha(0.65f));
				g.fillRect((float) pixelPos - 1.5f, 0.0f, 3.0f, (float) getHeight());
				g.setColour(Colours::lightgrey.withAlpha(0.3f));
			} else {
				g.drawVerticalLine(pixelPos, 0, getHeight());
			}
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

	if (automationLane != nullptr) {
		// Lane überspannt den ganzen Track (außer dem unteren Resize-Griff).
		automationLane->setBounds(0, 0, getWidth(), juce::jmax(0, getHeight() - 5));
		if (automationVisible)
			automationLane->toFront(false);
	}

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

void Track::mouseDrag(const juce::MouseEvent& e)
{
	// Nur reagieren, wenn das Event tatsaechlich vom Bottom-Resizer kommt und
	// Alt/Option gehalten wird. resizer treibt waehrend des Drags this->setBounds
	// → Track::resized() wurde also bereits ausgefuehrt und getHeight() hat den
	// neuen Wert. Wir spiegeln diesen einfach auf alle Geschwistertracks.
	if (e.eventComponent != resizer || !e.mods.isAltDown() || navigator == nullptr)
		return;

	// Re-Entry-Schutz: wenn other->setSize() wider Erwarten Mouse-Events
	// nachzieht, brechen wir hier ab statt eine Endlosschleife zu starten.
	static thread_local bool propagating = false;
	if (propagating) return;
	propagating = true;

	const int newH = getHeight();
	for (auto* other : navigator->getTracks()) {
		if (other == nullptr || other == this) continue;
		if (other->getHeight() != newH)
			other->setSize(other->getWidth(), newH);
	}

	// Y-Positionen aller Tracks live neu stapeln. Ohne diesen Schritt
	// behaelt der gezogene Track seine alte Y-Koordinate — die anderen
	// haben aber schon die neue Hoehe und ueberlappen ihn visuell (bzw.
	// es klafft eine Luecke darueber). Erst Mouse-Up wuerde sonst die
	// volle Reflow-Logik via parentSizeChanged / adjustHeight triggern.
	navigator->relayoutTracks();

	propagating = false;
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

// ----- Lane-Automation -------------------------------------------------------

void Track::setAutomationVisible(bool visible)
{
	this->automationVisible = visible;
	if (automationLane != nullptr) {
		automationLane->setVisible(visible);
		automationLane->setOverlayActive(visible);
		if (visible)
			automationLane->toFront(false);
	}
	repaint();
}

// Slot-Konstanten.
const char* Track::kInstrumentSlotName = "instr";

// Lokale Helfer: Plugin-eigene paramId (ohne Slot-Prefix).
static juce::String localParamId(juce::AudioProcessorParameter* p, int indexInPlugin)
{
	if (p == nullptr) return {};
	if (auto* withId = dynamic_cast<juce::AudioProcessorParameterWithID*>(p))
		return withId->paramID;
	return juce::String(indexInPlugin) + ":" + p->getName(64);
}

static juce::String scopedParamId(const juce::String& slot, const juce::String& localId)
{
	return slot + ":" + localId;
}

void Track::setAutomationParameterIndex(int idx)
{
	// Backward-compatible: bezieht sich auf das Instrument-Plugin. Delegiert
	// an setAutomationTarget().
	this->automationParamIndex = idx;
	if (automationLane != nullptr)
		automationLane->setParameterIndex(idx);

	if (idx < 0 || pluginModule == nullptr || pluginModule->getPlugin() == nullptr)
	{
		automationCurrentSlot.clear();
		automationCurrentParamId.clear();
		if (automationLane != nullptr) automationLane->setBoundLane(nullptr);
		return;
	}
	const auto& params = pluginModule->getPlugin()->getParameters();
	if (idx >= params.size()) return;
	const juce::String local = localParamId(params[idx], idx);
	if (local.isEmpty()) return;
	setAutomationTarget(juce::String(kInstrumentSlotName), local);
}

void Track::setAutomationTarget(const juce::String& slot, const juce::String& paramIdInPlugin)
{
	if (slot.isEmpty() || paramIdInPlugin.isEmpty())
	{
		automationCurrentSlot.clear();
		automationCurrentParamId.clear();
		if (automationLane != nullptr) automationLane->setBoundLane(nullptr);
		return;
	}
	const juce::String full = scopedParamId(slot, paramIdInPlugin);
	automationCurrentSlot     = slot;
	automationCurrentParamId  = full;
	auto& lane = automationModel.findOrCreate(full);
	if (automationLane != nullptr) automationLane->setBoundLane(&lane);
}

std::vector<std::pair<juce::String, PluginModule*>> Track::getAutomationTargets() const
{
	std::vector<std::pair<juce::String, PluginModule*>> out;
	if (pluginModule != nullptr && pluginModule->getPlugin() != nullptr)
		out.emplace_back(kInstrumentSlotName, pluginModule);
	for (int i = 0; i < NUM_FX_SLOTS; ++i)
		if (fxPlugins[i] != nullptr && fxPlugins[i]->getPlugin() != nullptr)
			out.emplace_back(fxSlotName(i), fxPlugins[i]);
	return out;
}

void Track::setFxPlugin(int slot, PluginModule* pm)
{
	if (slot < 0 || slot >= NUM_FX_SLOTS) return;
	// reconstructFXChainFromGraph haengt das Instrument-Plugin als ersten
	// Eintrag in den panel->fxChain — fuer den Track ist das aber bereits
	// ueber pluginModule erreichbar. Doppelt im Cache → doppelte
	// Parameter-Eintraege. Hier herausfiltern.
	if (pm == pluginModule) pm = nullptr;
	if (fxPlugins[slot] == pm) return;
	fxPlugins[slot] = pm;
	rebuildAutomationParamIdCache();
}

void Track::rebuildAutomationParamIdCache()
{
	cachedAutomationParams.clear();
	for (const auto& kv : getAutomationTargets())
	{
		const juce::String& slot = kv.first;
		PluginModule* pm = kv.second;
		if (pm == nullptr || pm->getPlugin() == nullptr) continue;
		const auto& params = pm->getPlugin()->getParameters();
		for (int i = 0; i < params.size(); ++i)
		{
			auto* p = params[i];
			if (p == nullptr) continue;
			auto entry = std::make_unique<CachedAutomationParam>();
			entry->scopedId  = scopedParamId(slot, localParamId(p, i));
			entry->parameter = p;
			cachedAutomationParams.push_back(std::move(entry));
		}
	}
}

void Track::applyPendingAutomation()
{
	// Message-Thread: jetzt darf gefahrlos ans Plugin-Parameter geschrieben
	// werden. Plugin-UIs reagieren in ihrem normalen Listener-Pfad, statt
	// synchron aus dem Audio-Callback heraus.
	for (auto& entry : cachedAutomationParams)
	{
		if (entry == nullptr || entry->parameter == nullptr) continue;
		// exchange holt den Pending-Flag und setzt ihn in einem Schritt zurueck
		// — kein Verlust, wenn der Audio-Thread parallel neue Werte schreibt.
		if (!entry->hasPending.exchange(false, std::memory_order_acquire))
			continue;
		const float v = entry->pendingValue.load(std::memory_order_relaxed);
		// Doppel-Pushes auf identische Werte sparen wir uns — manche Plugins
		// notifyien bei jedem Aufruf ihre UI, das macht im Idle nur unnoetig
		// Last.
		if (v == entry->lastApplied) continue;
		entry->lastApplied = v;
		entry->parameter->setValueNotifyingHost(v);
	}
}

juce::String Track::automationParameterIdFor(int idx) const
{
	// Liefert die GESCOPETE paramId fuer den Instrument-Slot — Aufrufer wie
	// TrackControl::populateParamBox brauchen weiterhin diese Bequemlichkeit.
	if (idx < 0 || pluginModule == nullptr || pluginModule->getPlugin() == nullptr)
		return {};
	const auto& params = pluginModule->getPlugin()->getParameters();
	if (idx >= params.size()) return {};
	const auto local = localParamId(params[idx], idx);
	if (local.isEmpty()) return {};
	return scopedParamId(kInstrumentSlotName, local);
}

// ----- AutomationLane Glue (hier definiert, weil Zugriff auf PluginModule) ---

juce::AudioProcessorParameter* AutomationLane::getTargetParameter() const
{
	if (track == nullptr || paramIndex < 0)
		return nullptr;
	auto* pm = track->getPluginModule();
	if (pm == nullptr || pm->getPlugin() == nullptr)
		return nullptr;
	const auto& params = pm->getPlugin()->getParameters();
	if (paramIndex >= params.size())
		return nullptr;
	return params[paramIndex];
}

// applyAt() ist obsolet — Track::setSamplePosition iteriert jetzt selbst alle
// Lanes des Modells. Die Methode bleibt als no-op fuer eventuelle alte
// Aufrufer und fuer zukuenftige UI-Spezialfaelle.
void AutomationLane::applyAt(long sample)
{
	juce::ignoreUnused(sample);
}
