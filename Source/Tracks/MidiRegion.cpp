/*
  ==============================================================================

    MidiRegion.cpp
    Created: 28 Dec 2016 6:55:16pm
    Author:  Matthias Pueski
    Modified: Mit globalem Tempo-Support

  ==============================================================================
*/

#include "MidiRegion.h"
#include "Track.h"
#include "TrackNavigator.h"

MidiRegion::MidiRegion(long startSample, long sampleLength, double sampleRate, MultiComponentDragger* dragger) {
    this->dragger = dragger;
    this->midiBuffer = new MidiBuffer();
    this->midiThumbnail = new MidiThumbnail(sampleRate);
    this->sampleRate = sampleRate;
    // Align to bufferSize so the raw sampleOffset member matches what
    // getSampleOffset() returns. Otherwise visual rendering (uses raw
    // sampleOffset) and MIDI dispatch / timestamping (uses the aligned
    // getSampleOffset) drift apart by up to bufferSize-1 samples — invisible
    // at low zoom, but at high zoom that gap turns into many pixels of
    // apparent note misalignment.
    {
        const int bs = Project::getInstance()->getBufferSize();
        this->sampleOffset = bs > 0 ? (startSample / bs) * bs : startSample;
    }
    this->name = "new";
    this->zoom = 40;
    this->numSamples = sampleLength;

    // Tempo-Parameter initialisieren
    this->currentBPM = 120.0; // Standard BPM
    this->quantizeGridBeats = 0.25; // 16tel-Noten
    this->snapToGrid = false;

    // Projekttempo übernehmen falls verfügbar
    updateTempoFromProject();

    double lengthInSeconds = numSamples / sampleRate;
    setBounds(sampleOffset / sampleRate * zoom, 0, lengthInSeconds * zoom, Project::DEFAULT_TRACK_HEIGHT);

    this->offset = 0;
    midiMessageList = std::vector<MidiMessage*>();
    this->sequence = new MidiMessageSequence();

    this->constrainer = new ResizeConstrainer();
    constrainer->setMinimumWidth(20);
    constrainer->setMaximumWidth(1000000);

    resizerR = new ResizableEdgeComponent(this, constrainer, ResizableEdgeComponent::rightEdge);
    resizerR->setSize(8, getHeight());
    addAndMakeVisible(resizerR);
    resizerL = new ResizableEdgeComponent(this, constrainer, ResizableEdgeComponent::leftEdge);
    resizerL->setSize(8, getHeight());
    addAndMakeVisible(resizerL);

    addComponentListener(this);

    auto& bus = FastBus::getInstance();

    tempoSubscriptionAlive = std::make_shared<bool>(true);
    std::weak_ptr<bool> weakAlive = tempoSubscriptionAlive;
    bus.subscribe<float>("tempo", [this, weakAlive](const FastTopic<float>& topic) {
        if (weakAlive.lock())
            handleTempoChange(topic.getValue());
    });

}

MidiRegion::MidiRegion(MidiBuffer* buffer, double sampleRate) {
    this->midiBuffer = buffer;
    this->midiThumbnail = new MidiThumbnail(sampleRate);
    this->sampleRate = sampleRate;
    this->name = "copy";
    this->zoom = 1;
    this->numSamples = buffer->getNumEvents();

    // Tempo-Parameter initialisieren
    this->currentBPM = 120.0;
    this->quantizeGridBeats = 0.25;
    this->snapToGrid = false;

    updateTempoFromProject();

    double lengthInSeconds = numSamples / sampleRate;
    setBounds(0, 0, lengthInSeconds * zoom, Project::DEFAULT_TRACK_HEIGHT);

    this->offset = 0;
    midiMessageList = std::vector<MidiMessage*>();
    this->sequence = new MidiMessageSequence();

    this->constrainer = new ResizeConstrainer();
    constrainer->setMinimumWidth(20);
    constrainer->setMaximumWidth(getBounds().getWidth());

    resizerR = new ResizableEdgeComponent(this, constrainer, ResizableEdgeComponent::rightEdge);
    resizerR->setSize(8, getHeight());
    addAndMakeVisible(resizerR);
    resizerL = new ResizableEdgeComponent(this, constrainer, ResizableEdgeComponent::leftEdge);
    resizerL->setSize(8, getHeight());
    addAndMakeVisible(resizerL);

    addComponentListener(this);

    auto& bus = FastBus::getInstance();

    tempoSubscriptionAlive = std::make_shared<bool>(true);
    std::weak_ptr<bool> weakAlive = tempoSubscriptionAlive;
    bus.subscribe<float>("tempo", [this, weakAlive](const FastTopic<float>& topic) {
        if (weakAlive.lock())
            handleTempoChange(topic.getValue());
    });

}

MidiRegion::~MidiRegion() {

    // Invalidate the tempo subscription sentinel so the lambda becomes a no-op.
    // Do NOT call removeTopic — that would remove all other MidiRegion subscribers.
    tempoSubscriptionAlive.reset();

    removeAllChangeListeners();

    // delete midi messages
    for (auto* m : midiMessageList) {
        delete m;
    }

    removeComponentListener(this);
    delete constrainer;
    delete midiThumbnail;
    delete midiBuffer;
    delete sequence;
}

void MidiRegion::mouseDoubleClick(const MouseEvent& /*e*/) {
    if (auto* nav = findParentComponentOfClass<TrackNavigator>())
        nav->openPianoRoll(this);
}

// Tempo-bezogene Methoden
void MidiRegion::updateTempoFromProject() {
    if (Project::getInstance()) {
        double projectBPM = Project::getInstance()->getTempo();
        if (projectBPM > 0) {
            currentBPM = projectBPM;
        }
    }
}

void MidiRegion::setTempo(double bpm) {
    if (bpm > 0) {
        currentBPM = bpm;
		calculateActualLength();
        repaint(); // Für BPM-Anzeige
    }
}

double MidiRegion::getCurrentBPM() const {
    return currentBPM;
}

long MidiRegion::beatsToSamples(double beats) const {
    double secondsPerBeat = 60.0 / currentBPM;
    return (long)(beats * secondsPerBeat * sampleRate);
}

double MidiRegion::samplesToBeats(long samples) const {
    double seconds = (double)samples / sampleRate;
    return seconds / (60.0 / currentBPM);
}

void MidiRegion::setQuantizeGrid(double beats) {
    quantizeGridBeats = beats;
}

double MidiRegion::getQuantizeGrid() const {
    return quantizeGridBeats;
}

void MidiRegion::setSnapToGrid(bool snap) {
    snapToGrid = snap;
    repaint();
}

bool MidiRegion::getSnapToGrid() const {
    return snapToGrid;
}

long MidiRegion::snapSampleToGrid(long sample) const {
    if (!snapToGrid) return sample;

    // Relativer Sample innerhalb der Region
    long relativeSample = sample - sampleOffset;
    double beats = samplesToBeats(relativeSample);
    double snappedBeats = std::round(beats / quantizeGridBeats) * quantizeGridBeats;
    long snappedSample = beatsToSamples(snappedBeats);

    return snappedSample + sampleOffset;
}

void MidiRegion::paint(Graphics& g) {
    // Track-Farbe als Region-Body — gleiche Logik wie bei AudioRegion.
    juce::Colour bodyColour(0xffffa500); // orange als Fallback
    if (auto* parentTrack = dynamic_cast<Track*>(getParentComponent()))
        bodyColour = parentTrack->getTrackColour();

    const bool sel = (dragger != nullptr) && dragger->isSelected(this);
    const juce::Colour fill = sel ? bodyColour.brighter(0.30f) : bodyColour.brighter(0.05f);
    const juce::Colour textCol = bodyColour.contrasting(0.85f);

    g.setColour(fill);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 10);

    g.setColour(bodyColour.darker(0.5f));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 10, 1.0f);

    // BPM-Info anzeigen
    g.setColour(textCol);
    g.setFont(Font(10.0f));
    String tempoInfo = String(currentBPM, 1) + " BPM";
    g.drawText(tempoInfo, getLocalBounds().removeFromTop(15), Justification::centredLeft);

    // Beat-Grid zeichnen (optional)
    if (snapToGrid && zoom > 20) { // Nur bei gutem Zoom sichtbar
        g.setColour(Colours::white.withAlpha(0.2f));

        double regionLengthBeats = samplesToBeats(numSamples);
        double beatsPerPixel = regionLengthBeats / getWidth();

        // Zeichne Grid-Linien für quantizeGridBeats
        for (double beat = 0; beat <= regionLengthBeats; beat += quantizeGridBeats) {
            int x = (int)(beat / beatsPerPixel);
            if (x < getWidth()) {
                g.drawVerticalLine(x, 15, getHeight());
            }
        }
    }

    g.setColour(juce::Colours::white.withAlpha(0.85f));
    if (midiThumbnail != nullptr && sampleRate > 0.0)
    {
        // Die sichtbaren Zeitgrenzen sind die Buffer-Position, an der die
        // linke Kante der Region steht (sourceStartSample), bis dahin plus
        // sichtbare Laenge. drawChannels filtert Events ausserhalb des
        // Bereichs aus und rendert die uebrigen relativ zu startTime — so
        // bleiben Noten beim Resize an ihrer Timeline-Position.
        const double startTime = (double) sourceStartSample / sampleRate;
        const double endTime   = (double) (sourceStartSample + getNumSamples()) / sampleRate;
        midiThumbnail->drawChannels(g, startTime, endTime, 1.0f);
    }
}

void MidiRegion::resized() {
    if (midiThumbnail != nullptr) {
        int x = (int)((sampleOffset / sampleRate) * zoom);
        int w = (int)((getNumSamples() / sampleRate) * zoom);
        midiThumbnail->setBounds(x, getY(), w, getHeight());
        midiThumbnail->setZoom(zoom);
    }
    if (resizerR != nullptr) {
        resizerR->setBounds(getWidth() - 8, 0, 8, getHeight());
    }
    if (resizerL != nullptr) {
        resizerL->setBounds(0, 0, 8, getHeight());
    }
}

void MidiRegion::updateThumb() {
    // Wenn waehrend einer laufenden Aufnahme die Region geloescht wird, ist
    // die Region beim Lambda-Aufruf moeglicherweise schon zerstoert. SafePointer
    // erkennt das und macht aus dem Crash einen No-Op.
    juce::Component::SafePointer<MidiRegion> safe(this);
    juce::MessageManager::callAsync([safe]()
    {
        auto* self = safe.getComponent();
        if (self == nullptr) return;

        const int x = (int)((self->sampleOffset / self->sampleRate) * self->zoom);
        const int w = (int)((self->getNumSamples() / self->sampleRate) * self->zoom);
        if (self->midiThumbnail != nullptr) {
            self->midiThumbnail->setBounds(x, self->getY(), w, self->getHeight());
            self->midiThumbnail->setZoom(self->zoom);
            self->midiThumbnail->repaint();
        }
        self->resized();
        self->repaint();
    });
}

void MidiRegion::componentMovedOrResized(Component& component, bool wasMoved, bool wasResized) {
    // A zoom change calls setBounds with new pixel coords derived from the
    // existing sampleOffset. Re-deriving sampleOffset from those pixels here
    // is not just redundant — rounding and snap-to-grid drift the offset on
    // every zoom step, which silently shifts recorded MIDI timestamps.
    if (zoomInProgress)
        return;

    // Consolidate hat eben Modell+Bounds finalisiert — der reentrant resize-
    // Handler darf hier nicht wieder snappen und das Right-Edge nach oben
    // verschieben (Folge waere ein wachsendes numSamples und Overlap mit
    // Nachbar nach Save/Reload).
    if (consolidateInProgress)
        return;

    Track* parentTrack = dynamic_cast<Track*>(getParentComponent());

    // Linkskanten-Resize triggert beides: wasMoved + wasResized. Die separaten
    // Branches unten wuerden setTopLeftPosition (Breite konstant, rechte Kante
    // wandert) und setSize (X konstant) hintereinander rufen — Region haut
    // nach rechts ab. Hier zusammen behandeln, rechte Kante aus den (vom
    // Constrainer schon korrigierten) bounds halten.
    if (wasMoved && wasResized) {
        const long currentLeftSample  = (long) std::round((double) getBounds().getX() / zoom * sampleRate);
        const long currentRightSample = (long) std::round((double)(getBounds().getX() + getBounds().getWidth()) / zoom * sampleRate);

        long snappedLeft = currentLeftSample;
        if (Project::getInstance()->isSnapToGridEnabled()) {
            const bool fine = juce::ModifierKeys::getCurrentModifiersRealtime().isShiftDown();
            snappedLeft = Project::getInstance()->snapSampleToGrid(snappedLeft, zoom, fine);
        }
        if (snappedLeft < 0) snappedLeft = 0;
        if (snappedLeft >= currentRightSample) snappedLeft = currentLeftSample;

        if (parentTrack != nullptr) {
            const long maxLeft = currentRightSample - 1;
            long candidate = parentTrack->clampOffsetForRegion(this, snappedLeft, currentRightSample - snappedLeft);
            if (candidate <= maxLeft) snappedLeft = candidate;
        }

        // Konstruktor + addMessage richten sampleOffset / Note-Timestamps an
        // bufferSize aus. Der Trim muss DENSELBEN aligned-Wert verwenden,
        // sonst rutschen sourceStartSample (uncorrected snap-delta) und
        // sampleOffset (aligned) auseinander — Folge: eine Note, die genau am
        // neuen Front-Edge liegen soll (Buffer-Position == sourceStartSample),
        // wird im Playback-Fenster knapp daneben gesucht und nicht gefunden.
        const int bs = Project::getInstance()->getBufferSize();
        const long snappedAligned = bs > 0 ? (snappedLeft / bs) * bs : snappedLeft;
        const long deltaLeft = snappedAligned - sampleOffset;

        const long newNumSamples_ = currentRightSample - snappedAligned;
        const int newX     = (int) std::round((double) snappedAligned / sampleRate * zoom);
        const int newWidth = (int) std::round((double) newNumSamples_ / sampleRate * zoom);

        // WICHTIG: erst Modell aktualisieren, dann setBounds. setBounds ruft
        // componentMovedOrResized reentrant auf — wenn dort sampleOffset noch
        // alt waere, wuerde deltaLeft erneut berechnet und sourceStartSample
        // ein zweites Mal erhoeht. Mit Modell-First wird beim Reentry
        // deltaLeft = 0 -> idempotent.
        sampleOffset       = snappedAligned;
        numSamples         = newNumSamples_;
        sourceStartSample += deltaLeft;
        if (sourceStartSample < 0) sourceStartSample = 0;

        if (newX != getBounds().getX() || newWidth != getWidth()) {
            setBounds(newX, getY(), newWidth, getHeight());
        }

        rebuildThumbnail();
        updateThumb();
        return;
    }

    if (wasMoved) {
        const long rawSampleOffset = std::round((getBounds().getX() / zoom) * sampleRate);
        long newSampleOffset = rawSampleOffset;

        // Multi-Drag-Folger: Snap/Clamp pro Region wuerde die Gruppe
        // auseinanderziehen — der Leader hat schon gerastert, alle anderen
        // bekommen vom Dragger den gleichen effektiven Pixel-Delta.
        const bool isMultiFollower =
            dragger != nullptr
            && dragger->isMultiDragInProgress()
            && dragger->getComponentBeingDragged() != this;

        if (!isMultiFollower) {
            // Snap der Region auf das Taktraster, wenn global aktiv. Vorrang vor
            // dem feineren MIDI-Note-Quantisierungsraster (snapSampleToGrid),
            // weil das hier den Region-Anfang und nicht eine einzelne Note betrifft.
            // Granularität: bei zoom >= 100 auf 1/4 Takt, sonst ganze Takte.
            // Mit gehaltener Shift-Taste: 1/4 des aktuellen Rasters für feinere
            // Platzierung.
            if (Project::getInstance()->isSnapToGridEnabled()) {
                const bool fine = juce::ModifierKeys::getCurrentModifiersRealtime().isShiftDown();
                newSampleOffset = Project::getInstance()->snapSampleToGrid(newSampleOffset, zoom, fine);
            }

            // Overlap-Clamp gegen die Nachbarregionen.
            if (parentTrack != nullptr)
                newSampleOffset = parentTrack->clampOffsetForRegion(this, newSampleOffset, numSamples);
        }

        // Bounds nur dann visuell korrigieren, wenn snap/clamp die Position
        // tatsächlich verändert hat — sonst entsteht wegen Pixel-Rundung
        // Ping-Pong durch das recursive componentMovedOrResized.
        if (newSampleOffset != rawSampleOffset) {
            const double lengthInSeconds = numSamples / sampleRate;
            setBounds((int)std::round(newSampleOffset / sampleRate * zoom), getY(),
                      (int)std::round(lengthInSeconds * zoom), getHeight());
        }

        // Align so raw sampleOffset matches getSampleOffset(); see constructor.
        const int bs = Project::getInstance()->getBufferSize();
        sampleOffset = bs > 0 ? (newSampleOffset / bs) * bs : newSampleOffset;

        rebuildThumbnail();
        updateThumb();
    }

    if (wasResized) {
        long newNumSamples = (long)std::round((double)getWidth() / zoom * sampleRate);

        // Snap der rechten Kante auf das Taktraster, wenn global aktiv —
        // gleiches Verhalten wie beim Verschieben der Region. Mit gehaltener
        // Shift-Taste 1/4 des aktuellen Rasters für feinere Platzierung.
        if (Project::getInstance()->isSnapToGridEnabled()) {
            const bool fine = juce::ModifierKeys::getCurrentModifiersRealtime().isShiftDown();
            const long rightEdge = sampleOffset + newNumSamples;
            const long snappedRightEdge = Project::getInstance()->snapSampleToGrid(rightEdge, zoom, fine);
            const long snappedLen = snappedRightEdge - sampleOffset;
            if (snappedLen > 0)
                newNumSamples = snappedLen;
        }

        // Begrenze die Länge so, dass die rechte Kante nicht in die nächste Region läuft.
        if (parentTrack != nullptr) {
            const long maxLen = parentTrack->clampLengthForRegion(this, sampleOffset, newNumSamples);
            if (maxLen < newNumSamples)
                newNumSamples = maxLen;
        }

        const int newWidth = (int)std::round((double)newNumSamples / sampleRate * zoom);
        if (newWidth != getWidth())
            setSize(newWidth, getHeight());

        if (newNumSamples > 0)
            numSamples = newNumSamples;
        updateThumb();
    }
}

void MidiRegion::setNumSamples(int numSamples) {
    this->numSamples = numSamples;
    setBounds(getX(), getY(), (int)std::round(numSamples / sampleRate * zoom), getHeight());
    updateThumb();
}

int MidiRegion::getNumSamples() {
    return numSamples;
}

MidiBuffer* MidiRegion::getBuffer() {
    return midiBuffer;
}

MidiMessageSequence* MidiRegion::getSequence() {
    return sequence;
}

MidiThumbnail* MidiRegion::getThumbnail() {
    return midiThumbnail;
}

void MidiRegion::setZoom(float newZoom) {
    this->zoom = newZoom;

    // Bounds mit neuem Zoom aktualisieren
    double lengthInSeconds = numSamples / sampleRate;
    zoomInProgress = true;
    setBounds((int)std::round(sampleOffset / sampleRate * zoom), getY(),
        (int)std::round(lengthInSeconds * zoom), getHeight());
    zoomInProgress = false;

    // Constrainer maxWidth an neuen Zoom anpassen, damit kein Sprung beim Resize
    int maxW = (int)(Project::getInstance()->getTrackLength() * zoom);
    constrainer->setMaxWidth(maxW > 0 ? maxW : 1000000);

    this->midiThumbnail->setZoom(zoom);
    updateThumb();
}

void MidiRegion::setLoop(bool loop) {
    this->loop = loop;
    repaint();
}

void MidiRegion::clear() {
    midiBuffer->clear();
    sequence->clear();
}

void MidiRegion::addMessageRaw(juce::MidiMessage* m, juce::int64 bufferTimestamp)
{
    if (m == nullptr) return;
    ScopedLock c(lock);
    m->setTimeStamp((double) bufferTimestamp);
    midiMessageList.push_back(m);
    if (midiThumbnail != nullptr)
        midiThumbnail->addMessage((int) bufferTimestamp, m);
}

void MidiRegion::addMessage(MidiMessage* m, double time, int sampleNum) {

    // sampleNum -= sampleNum % Project::getInstance()->getBufferSize();

    ScopedLock c(lock);

    // Optional: Message an Grid snappen
    if (snapToGrid) {
        int relativeSample = sampleNum - getSampleOffset();
        int snappedSample = snapSampleToGrid(relativeSample + getSampleOffset()) - getSampleOffset();
        sampleNum = snappedSample + getSampleOffset();
    }

    // Timestamp ist buffer-relativ. Buffer-Position 0 entspricht der
    // ORIGINAL-linken-Kante der Region (vor jedem Links-Trim) — nach einem
    // Links-Trim ist die sichtbare linke Kante (sampleOffset) bei Buffer-
    // Position sourceStartSample. Eine neue Note an Timeline=sampleNum hat
    // also Buffer-Timestamp = (sampleNum - sampleOffset) + sourceStartSample.
    // Ohne sourceStartSample-Korrektur landete die Note bei Buffer-Position
    // 0..N, waehrend Playback/Draw ab sourceStartSample lesen — sie wuerde
    // ignoriert und beim Consolidate ausgefiltert.
    const int relSample   = sampleNum - getSampleOffset();
    const int bufTimeStamp = relSample + (int) sourceStartSample;
    m->setTimeStamp(bufTimeStamp);

    midiMessageList.push_back(m);

    if (relSample > numSamples) {
        setNumSamples(relSample);
    }

    midiThumbnail->addMessage(bufTimeStamp, m);
    updateThumb();
}

vector<MidiMessage*> MidiRegion::getActiveMessages(int localSamplePos) {
    vector<MidiMessage*> result;

    // Wenn Position zurückspringt (nach Verschieben), resette aktive Notes
    if (localSamplePos < lastPlaybackPosition) {
        activeNotes.clear();
    }

    // Finde alle Messages zwischen letzter und aktueller Position
    int searchStart = std::max(0, lastPlaybackPosition);
    int searchEnd = localSamplePos + 64;

    for (auto* m : midiMessageList) {
        if (!m) continue;

        int msgTime = (int)m->getTimeStamp();

        // Message liegt in unserem Suchbereich
        if (msgTime >= searchStart && msgTime < searchEnd) {
            if (m->isNoteOn()) {
                activeNotes[m->getNoteNumber()] = m;
                if (msgTime >= localSamplePos) {
                    result.push_back(m); // Neue Note-On
                }
            }
            else if (m->isNoteOff()) {
                activeNotes.erase(m->getNoteNumber());
                if (msgTime >= localSamplePos) {
                    result.push_back(m); // Note-Off
                }
            }
            else if (msgTime >= localSamplePos) {
                result.push_back(m); // Controller etc.
            }
        }
    }

    // Für noch aktive Notes: sende "phantom" Note-On Messages
    if (localSamplePos != lastPlaybackPosition) {
        for (auto& pair : activeNotes) {
            result.push_back(pair.second);
        }
    }

    lastPlaybackPosition = localSamplePos;
    return result;
}

vector<MidiMessage*> MidiRegion::getMessages(int sampleNum, int numSamples)
{
    // Der Aufrufer uebergibt sampleNum relativ zur Region (0 = linke Kante).
    // Im Buffer beginnt die linke Kante bei sourceStartSample — also den
    // Offset hier dazurechnen, sonst wuerden Noten nach Links-Trim falsche
    // Buffer-Positionen referenzieren.
    vector<MidiMessage*> foundMessages;
    const int startInBuffer = sampleNum  + (int) sourceStartSample;
    const int endInBuffer   = startInBuffer + numSamples;

    for (auto* m : midiMessageList)
    {
        if (m != nullptr &&
            m->getTimeStamp() >= startInBuffer &&
            m->getTimeStamp() < endInBuffer &&
            (m->isNoteOnOrOff() || m->isMidiClock() || m->isController()))
        {
            foundMessages.push_back(m);
        }
    }

    return foundMessages;
}

void MidiRegion::startRecording() {
    startTimer(100);
    currentMessageNum = 0;
}

void MidiRegion::rebuildThumbnail() {
    midiThumbnail->removeAllMessages();
    for (auto* m : midiMessageList) {
        if (m != nullptr) {
            int relSample = static_cast<int>(m->getTimeStamp());
            midiThumbnail->addMessage(relSample, m);
        }
    }
}

void MidiRegion::stopRecording() {
    stopTimer();
    // Send all-notes-off for all 16 MIDI channels
    for (int channel = 1; channel <= 16; channel++) {
        midiBuffer->addEvent(MidiMessage::allNotesOff(channel), numSamples);
    }
}

void MidiRegion::timerCallback() {
    updateThumb();
}

void MidiRegion::reset() {
    currentMessageNum = 0;
}

int MidiRegion::calculateActualLength() const {
    int dataLength = 0;
    for (auto* msg : midiMessageList) {
        if (msg) {
            int msgTime = static_cast<int>(msg->getTimeStamp());
            dataLength = std::max(dataLength, msgTime);
        }
    }

    // Verwende visuelle Länge als Fallback
    int visualLength = static_cast<int>((getBounds().getWidth() / zoom) * sampleRate);

    return std::max({ dataLength, visualLength, 1 });
}

void MidiRegion::setMidiChannelForAllMessages(int channel) {
    if (channel < 1 || channel > 16)
        return;

    const ScopedLock sl(lock);

    for (auto* msg : midiMessageList) {
        if (msg != nullptr && msg->getChannel() > 0)
            msg->setChannel(channel);
    }

    // sequence wird beim Bouncen/Export verwendet — auch dort den Channel
    // mitziehen, sonst sind die exportierten MIDI-Files inkonsistent zum
    // hörbaren Playback.
    if (sequence != nullptr) {
        for (int i = 0; i < sequence->getNumEvents(); ++i) {
            auto* ev = sequence->getEventPointer(i);
            if (ev != nullptr && ev->message.getChannel() > 0)
                ev->message.setChannel(channel);
        }
    }
}

MidiRegion* MidiRegion::clone(long newOffset) {
    const ScopedLock sl(lock);

    // Verwende die berechnete tatsächliche Länge
    int actualLength = calculateActualLength();

    // Erstelle Klon mit korrekter Länge
    auto* copy = new MidiRegion(newOffset, actualLength, this->sampleRate, this->dragger);

    // Tempo-Einstellungen kopieren
    copy->setTempo(this->currentBPM);
    copy->setQuantizeGrid(this->quantizeGridBeats);
    copy->setSnapToGrid(this->snapToGrid);

    // Kopiere alle MIDI-Messages
    for (auto* msg : midiMessageList) {
        if (msg) {
            auto* clonedMsg = new MidiMessage(*msg);
            clonedMsg->setTimeStamp(clonedMsg->getTimeStamp());
            copy->midiMessageList.push_back(clonedMsg);

            // Füge zur Thumbnail hinzu
            copy->midiThumbnail->addMessage(static_cast<int>(clonedMsg->getTimeStamp()), clonedMsg);
        }
    }

    // Setze die korrekte Größe und aktualisiere
    copy->setZoom(this->zoom);
    copy->setNumSamples(actualLength);
    copy->rebuildThumbnail();
    copy->updateThumb();

    return copy;
}

std::pair<MidiRegion*, MidiRegion*> MidiRegion::splitAt(long splitSample, MultiComponentDragger* dragger) {
    const ScopedLock sl(lock);

    long relativeSplit = splitSample - sampleOffset;
    if (relativeSplit <= 0 || relativeSplit >= numSamples)
        return { nullptr, nullptr };

    // Neue Regionen
    auto* left = new MidiRegion(sampleOffset, relativeSplit, sampleRate, dragger);
    auto* right = new MidiRegion(splitSample, numSamples - relativeSplit, sampleRate, dragger);

    // Tempo-Einstellungen übertragen (snapToGrid erst NACH dem Hinzufügen
    // der Messages setzen, damit addMessage die Timestamps nicht verändert)
    left->setTempo(currentBPM);
    left->setQuantizeGrid(quantizeGridBeats);

    right->setTempo(currentBPM);
    right->setQuantizeGrid(quantizeGridBeats);

    // Paare tracken
    std::multimap<int, MidiMessage*> noteOnMap;

    for (auto* msg : midiMessageList) {
        if (!msg)
            continue;

        int t = static_cast<int>(msg->getTimeStamp());
        int note = msg->getNoteNumber();

        if (msg->isNoteOn()) {
            noteOnMap.insert({ note, msg });
        }
        else if (msg->isNoteOff()) {
            auto range = noteOnMap.equal_range(note);
            for (auto it = range.first; it != range.second; ++it) {
                auto* on = it->second;
                int onTime = static_cast<int>(on->getTimeStamp());
                int offTime = t;

                // Fall A: Note liegt komplett links
                if (offTime <= relativeSplit) {
                    auto* onCopy = new MidiMessage(*on);
                    auto* offCopy = new MidiMessage(*msg);
                    onCopy->setTimeStamp(onTime);
                    offCopy->setTimeStamp(offTime);
                    left->addMessage(onCopy, 0, onTime + sampleOffset);
                    left->addMessage(offCopy, 0, offTime + sampleOffset);
                }

                // Fall B: Note überlappt den Split
                else if (onTime < relativeSplit && offTime > relativeSplit) {
                    // Linke Seite: gestutzte Note mit Note-Off am Split
                    auto* onCopy = new MidiMessage(*on);
                    auto* offCopy = new MidiMessage(MidiMessage::noteOff(on->getChannel(), on->getNoteNumber()));
                    onCopy->setTimeStamp(onTime);
                    offCopy->setTimeStamp(relativeSplit);
                    left->addMessage(onCopy, 0, onTime + sampleOffset);
                    left->addMessage(offCopy, 0, relativeSplit + sampleOffset);

                    // Rechte Seite: neue Note-On ab Split mit verkürzter Dauer
                    auto* newOn = new MidiMessage(*on);
                    newOn->setTimeStamp(0); // ab Beginn rechter Region
                    int newDuration = offTime - relativeSplit;
                    auto* newOff = new MidiMessage(*msg);
                    newOff->setTimeStamp(newDuration);
                    right->addMessage(newOn, 0, splitSample);
                    right->addMessage(newOff, 0, splitSample + newDuration);
                }

                // Fall C: Note komplett rechts
                else if (onTime >= relativeSplit) {
                    auto* onCopy = new MidiMessage(*on);
                    auto* offCopy = new MidiMessage(*msg);
                    int relOn = onTime - relativeSplit;
                    int relOff = offTime - relativeSplit;
                    onCopy->setTimeStamp(relOn);
                    offCopy->setTimeStamp(relOff);
                    right->addMessage(onCopy, 0, relOn + splitSample);
                    right->addMessage(offCopy, 0, relOff + splitSample);
                }

                noteOnMap.erase(it);
                break;
            }
        }
    }

    // Andere Events (Controller, Clock, SysEx etc.)
    for (auto* msg : midiMessageList) {
        if (!msg || msg->isNoteOnOrOff())
            continue;

        int t = static_cast<int>(msg->getTimeStamp());
        if (t < relativeSplit) {
            auto* copy = new MidiMessage(*msg);
            copy->setTimeStamp(t);
            left->addMessage(copy, 0, t + sampleOffset);
        }
        else {
            auto* copy = new MidiMessage(*msg);
            int relT = t - relativeSplit;
            copy->setTimeStamp(relT);
            right->addMessage(copy, 0, relT + splitSample);
        }
    }

    // Snap erst jetzt aktivieren, nachdem alle Messages hinzugefügt wurden
    left->setSnapToGrid(snapToGrid);
    right->setSnapToGrid(snapToGrid);

    // Zoom und Bounds
    left->setZoom(zoom);
    right->setZoom(zoom);

    left->setNumSamples(relativeSplit);
    right->setNumSamples(numSamples - relativeSplit);

    left->rebuildThumbnail();
    right->rebuildThumbnail();

    left->updateThumb();
    right->updateThumb();

	left->setName(name + "_L");
	right->setName(name + "_R");

    return { left, right };
}

void MidiRegion::consolidate()
{
    const ScopedLock sl(lock);

    // Erst den Front-Trim einarbeiten, falls die Region links gekuerzt wurde:
    // Events vor sourceStartSample wegwerfen, alle uebrigen um -sourceStartSample
    // verschieben, dann sourceStartSample auf 0 zuruecksetzen. Ohne diesen
    // Schritt wuerden Noten am links-getrimmten Rand (mit Buffer-Timestamp ==
    // sourceStartSample) den darunter liegenden Trim-Filter im non-looping-
    // Branch nicht ueberleben — sie liegen ja per Konstruktion VOR dem
    // sichtbaren Bereich, gemessen vom alten Buffer-Anfang aus.
    if (sourceStartSample > 0)
    {
        const long sss = sourceStartSample;

        // Vor dem Cull noteOn/noteOff paaren: laeuft eine Note ueber die neue
        // linke Kante (on < sss, off >= sss), das noteOn auf sss ziehen — sonst
        // verschwindet es im naechsten remove_if und der noteOff bleibt als
        // haengende Note zurueck.
        {
            std::map<int, MidiMessage*> openOn;
            for (auto* m : midiMessageList) {
                if (m == nullptr) continue;
                if (m->isNoteOn()) {
                    openOn[(m->getChannel() - 1) * 128 + m->getNoteNumber()] = m;
                } else if (m->isNoteOff()) {
                    const int key = (m->getChannel() - 1) * 128 + m->getNoteNumber();
                    auto it = openOn.find(key);
                    if (it != openOn.end()) {
                        MidiMessage* on = it->second;
                        if ((long) on->getTimeStamp() < sss && (long) m->getTimeStamp() >= sss)
                            on->setTimeStamp((double) sss);
                        openOn.erase(it);
                    }
                }
            }
        }

        midiMessageList.erase(
            std::remove_if(midiMessageList.begin(), midiMessageList.end(),
                [sss](MidiMessage* m) {
                    if (m == nullptr) return true;
                    if ((long) m->getTimeStamp() < sss) {
                        delete m;
                        return true;
                    }
                    return false;
                }),
            midiMessageList.end());
        for (auto* m : midiMessageList)
            if (m != nullptr)
                m->setTimeStamp(m->getTimeStamp() - (double) sss);
        sourceStartSample = 0;
    }

    if (loop && loopCount > 0)
    {
        // Looping case: bake all repetitions into the MIDI data
        std::vector<MidiMessage*> origMessages(midiMessageList);

        for (int i = 1; i <= loopCount; i++)
        {
            for (auto* m : origMessages)
            {
                auto* copy = new MidiMessage(*m);
                copy->setTimeStamp(m->getTimeStamp() + (double)(i * numSamples));
                midiMessageList.push_back(copy);
            }
        }

        numSamples = numSamples * (loopCount + 1);
        loopCount  = 0;
        loop       = false;
    }
    else
    {
        // Non-looping case: alles ausserhalb des sichtbaren Bereichs IMMER
        // wegwerfen. Das ist der Sinn von Consolidate — Daten, die durch
        // Resize-vorne/-hinten "versteckt" wurden, werden jetzt endgueltig
        // entfernt. Front-Trim haben wir schon oben (und sourceStartSample
        // wurde auf 0 gesetzt), hier bleibt nur die rechte Kante.
        const long visibleEnd = (long) numSamples;

        // Noten, die innerhalb starten aber ueber die rechte Kante laufen,
        // muessen ihren noteOff in den sichtbaren Bereich gezogen bekommen
        // (visibleEnd - 1) — sonst killt der Cull den noteOff und die Note
        // haengt bis zum naechsten All-Notes-Off / Region-Ende.
        if (visibleEnd > 0)
        {
            const long clipped = visibleEnd - 1;
            std::map<int, MidiMessage*> openOn;
            for (auto* m : midiMessageList) {
                if (m == nullptr) continue;
                if (m->isNoteOn()) {
                    openOn[(m->getChannel() - 1) * 128 + m->getNoteNumber()] = m;
                } else if (m->isNoteOff()) {
                    const int key = (m->getChannel() - 1) * 128 + m->getNoteNumber();
                    auto it = openOn.find(key);
                    if (it != openOn.end()) {
                        MidiMessage* on = it->second;
                        if ((long) on->getTimeStamp() < visibleEnd && (long) m->getTimeStamp() >= visibleEnd)
                            m->setTimeStamp((double) clipped);
                        openOn.erase(it);
                    }
                }
            }
        }

        midiMessageList.erase(
            std::remove_if(midiMessageList.begin(), midiMessageList.end(),
                [visibleEnd](MidiMessage* m) {
                    if (m == nullptr) return true;
                    if ((long) m->getTimeStamp() >= visibleEnd) {
                        delete m;
                        return true;
                    }
                    return false;
                }),
            midiMessageList.end());
    }

    // Resize-Handler suppressen: ohne Flag wuerde wasResized im
    // componentMovedOrResized den right-edge snap erneut anwenden und
    // numSamples potenziell hoch-snappen — die Region wuerde wachsen und
    // nach Save/Reload den Nachbarn ueberlappen. Wir wissen hier exakt was
    // wir wollen (numSamples bereits final), also nur visuell gleichschalten.
    consolidateInProgress = true;
    setBounds(getX(), getY(), (int) std::round((double) numSamples / sampleRate * zoom), getHeight());
    consolidateInProgress = false;
    rebuildThumbnail();
    updateThumb();
    repaint();
}

void MidiRegion::quantize(int quantizeGridSamples)
{
    const ScopedLock sl(lock);

    std::multimap<int, MidiMessage*> noteOnMap;
    std::vector<MidiMessage*> quantizedMessages;
    std::vector<std::pair<MidiMessage*, MidiMessage*>> notePairs;

    // Schritt 1: Paare aus Note-On und Note-Off bilden
    for (auto* msg : midiMessageList)
    {
        if (!msg)
            continue;

        if (msg->isNoteOn())
        {
            noteOnMap.insert({ msg->getNoteNumber(), msg });
        }
        else if (msg->isNoteOff())
        {
            int note = msg->getNoteNumber();
            auto range = noteOnMap.equal_range(note);
            for (auto it = range.first; it != range.second; ++it)
            {
                if (it->second != nullptr)
                {
                    notePairs.emplace_back(it->second, msg);
                    noteOnMap.erase(it);
                    break;
                }
            }
        }
        else
        {
            // Andere MIDI-Nachrichten direkt quantisieren
            auto* quantized = new MidiMessage(*msg);
            int originalTime = (int)msg->getTimeStamp();
            int quantizedTime = ((originalTime + quantizeGridSamples / 2) / quantizeGridSamples) * quantizeGridSamples;
            quantized->setTimeStamp(quantizedTime);
            quantizedMessages.push_back(quantized);
        }
    }

    // Schritt 2: Note-Paare gemeinsam quantisieren
    for (auto& pair : notePairs)
    {
        auto* originalOn = pair.first;
        auto* originalOff = pair.second;

        int originalOnTime = (int)originalOn->getTimeStamp();
        int originalOffTime = (int)originalOff->getTimeStamp();
        int duration = originalOffTime - originalOnTime;

        int quantizedOnTime = ((originalOnTime + quantizeGridSamples / 2) / quantizeGridSamples) * quantizeGridSamples;

        auto* quantizedOn = new MidiMessage(*originalOn);
        auto* quantizedOff = new MidiMessage(*originalOff);

        quantizedOn->setTimeStamp(quantizedOnTime);
        quantizedOff->setTimeStamp(quantizedOnTime + duration);

        quantizedMessages.push_back(quantizedOn);
        quantizedMessages.push_back(quantizedOff);
    }

    // Alte Nachrichten löschen
    for (auto* msg : midiMessageList)
        delete msg;

    midiMessageList = quantizedMessages;

    rebuildThumbnail();
    updateThumb();
}

// Neue Beat-basierte Quantisierung
void MidiRegion::quantizeToBeats(double beatGrid) {
    const ScopedLock sl(lock);

    std::multimap<int, MidiMessage*> noteOnMap;
    std::vector<MidiMessage*> quantizedMessages;
    std::vector<std::pair<MidiMessage*, MidiMessage*>> notePairs;

    // Schritt 1: Paare aus Note-On und Note-Off bilden
    for (auto* msg : midiMessageList) {
        if (!msg) continue;

        if (msg->isNoteOn()) {
            noteOnMap.insert({ msg->getNoteNumber(), msg });
        }
        else if (msg->isNoteOff()) {
            int note = msg->getNoteNumber();
            auto range = noteOnMap.equal_range(note);
            for (auto it = range.first; it != range.second; ++it) {
                if (it->second != nullptr) {
                    notePairs.emplace_back(it->second, msg);
                    noteOnMap.erase(it);
                    break;
                }
            }
        }
        else {
            // Andere MIDI-Nachrichten zu Beats quantisieren
            auto* quantized = new MidiMessage(*msg);
            int originalSample = (int)msg->getTimeStamp();
            double originalBeats = samplesToBeats(originalSample);
            double quantizedBeats = std::round(originalBeats / beatGrid) * beatGrid;
            int quantizedSample = beatsToSamples(quantizedBeats);
            quantized->setTimeStamp(quantizedSample);
            quantizedMessages.push_back(quantized);
        }
    }

    // Schritt 2: Note-Paare gemeinsam quantisieren (Note-On position, Dauer beibehalten)
    for (auto& pair : notePairs) {
        auto* originalOn = pair.first;
        auto* originalOff = pair.second;

        int originalOnSample = (int)originalOn->getTimeStamp();
        int originalOffSample = (int)originalOff->getTimeStamp();
        int durationSamples = originalOffSample - originalOnSample;

        // Note-On Position quantisieren
        double originalOnBeats = samplesToBeats(originalOnSample);
        double quantizedOnBeats = std::round(originalOnBeats / beatGrid) * beatGrid;
        int quantizedOnSample = beatsToSamples(quantizedOnBeats);

        auto* quantizedOn = new MidiMessage(*originalOn);
        auto* quantizedOff = new MidiMessage(*originalOff);

        quantizedOn->setTimeStamp(quantizedOnSample);
        quantizedOff->setTimeStamp(quantizedOnSample + durationSamples); // Dauer beibehalten

        quantizedMessages.push_back(quantizedOn);
        quantizedMessages.push_back(quantizedOff);
    }

    // Alte Nachrichten löschen
    for (auto* msg : midiMessageList)
        delete msg;

    midiMessageList = quantizedMessages;

    rebuildThumbnail();
    updateThumb();
}

MidiMessageSequence MidiRegion::getMidiMessageSequence()
{
    MidiMessageSequence seq;
    for (auto* msg : midiMessageList)
    {
        if (msg != nullptr) {
            seq.addEvent(*msg);
        }
    }
    Logger::getCurrentLogger()->writeToLog("MidiRegion::getMidiMessageSequence: " + String(seq.getNumEvents()) + " events");

    seq.updateMatchedPairs();

    return seq;
}

void MidiRegion::setMidiMessageSequence(const MidiMessageSequence& seq)
{
    const ScopedLock sl(lock);

    // Alte Nachrichten löschen
    for (auto* msg : midiMessageList)
        delete msg;

    midiMessageList.clear();

    for (int i = 0; i < seq.getNumEvents(); ++i)
    {
        const MidiMessageSequence::MidiEventHolder* ev = seq.getEventPointer(i);
        if (ev != nullptr)
        {
            MidiMessage* copied = new MidiMessage(ev->message);
            midiMessageList.push_back(copied);
        }
    }

    rebuildThumbnail();
    updateThumb();
}

void MidiRegion::rescaleMidiEvents(double oldBPM, double newBPM) {

    const ScopedLock sl(lock);

    for (auto* msg : midiMessageList) {
        if (msg) {
            // Konvertiere Sample zu Beats mit ALTEM Tempo
            double oldSampleTime = msg->getTimeStamp();
            double seconds = oldSampleTime / sampleRate;
            double beats = seconds / (60.0 / oldBPM);

            // Konvertiere Beats zu Sample mit NEUEM Tempo
            double newSeconds = beats * (60.0 / newBPM);
            long newSample = (long)(newSeconds * sampleRate);

            msg->setTimeStamp(newSample);
        }
    }
}

void MidiRegion::handleTempoChange(double newBPM) {
    if (newBPM <= 0 || std::abs(newBPM - currentBPM) < 0.001) {
        return;
    }
    double oldBPM = this->currentBPM;
    this->currentBPM = newBPM;

    if (oldBPM > 0 && this->currentBPM > 0) {
        rescaleMidiEvents(oldBPM, this->currentBPM);

        // Keep musical length and position constant: scale samples by oldBPM/newBPM.
        // Without this an empty region would collapse to ~100ms (updateBoundsFromTempo
        // derives length from the last event), and the region's start position would
        // stay fixed in seconds — drifting away from its bar/beat anchor.
        const double scale = oldBPM / this->currentBPM;

        numSamples = std::max(1L, (long)std::llround((double)numSamples * scale));

        const long scaledOffset = (long)std::llround((double)sampleOffset * scale);
        const int bs = Project::getInstance()->getBufferSize();
        sampleOffset = bs > 0 ? (scaledOffset / bs) * bs : scaledOffset;

        const double lengthInSeconds = (double)numSamples / sampleRate;
        setBounds((int)std::round(sampleOffset / sampleRate * zoom), getY(),
                  (int)std::round(lengthInSeconds * zoom), getHeight());
    }

    rebuildThumbnail();
    updateThumb();
    constrainer->setMaximumWidth(getBounds().getWidth());
    repaint();
}

void MidiRegion::updateBoundsFromTempo() {
    // Finde die längste Note/Event in aktuellen Samples
    long maxSample = 0;
    for (auto* msg : midiMessageList) {
        if (msg) {
            long sample = static_cast<long>(msg->getTimeStamp());
            maxSample = std::max(maxSample, sample);
        }
    }

    // Minimum-Länge sicherstellen
    long newNumSamples = std::max(maxSample + (long)(sampleRate * 0.1), 1L); // 100ms Puffer

    if (newNumSamples != numSamples) {
        numSamples = newNumSamples;

        // Bounds entsprechend anpassen
        double lengthInSeconds = (double)numSamples / sampleRate;
        setBounds((int)(sampleOffset / sampleRate * zoom), getY(),
            (int)(lengthInSeconds * zoom), getHeight());
    }
}
// Add this method to the MidiRegion class (in MidiRegion.h and MidiRegion.cpp)

void MidiRegion::sanitize() {
    const ScopedLock sl(lock);

    // Map to track open notes: [channel][noteNumber] -> MidiMessage*
    std::map<int, std::map<int, MidiMessage*>> openNotes;
    std::vector<MidiMessage*> newMessages;

    // First pass: collect all existing messages and track open notes
    for (auto* msg : midiMessageList) {
        if (!msg) continue;

        if (msg->isNoteOn()) {
            int channel = msg->getChannel();
            int noteNumber = msg->getNoteNumber();

            // If there's already an open note on this channel/note, close it first
            if (openNotes[channel].find(noteNumber) != openNotes[channel].end()) {
                // Create note-off for the previous open note
                auto* noteOff = new MidiMessage(MidiMessage::noteOff(channel, noteNumber, (juce::uint8)0));
                noteOff->setTimeStamp(msg->getTimeStamp() - 1); // Place just before the new note-on
                newMessages.push_back(noteOff);
            }

            // Register this note as open
            openNotes[channel][noteNumber] = msg;
        }
        else if (msg->isNoteOff()) {
            int channel = msg->getChannel();
            int noteNumber = msg->getNoteNumber();

            // Remove from open notes if it exists
            if (openNotes[channel].find(noteNumber) != openNotes[channel].end()) {
                openNotes[channel].erase(noteNumber);
            }
            // If note-off without corresponding note-on, we'll keep it anyway
        }
    }

    // Find the end time of the region for closing remaining open notes
    long regionEndSample = numSamples;

    // Also check the actual last event time
    long lastEventTime = 0;
    for (auto* msg : midiMessageList) {
        if (msg) {
            lastEventTime = std::max(lastEventTime, (long)msg->getTimeStamp());
        }
    }

    // Use the later of region end or last event + small buffer
    long closeTime = std::max(regionEndSample, lastEventTime + (long)(sampleRate * 0.1)); // 100ms buffer

    // Second pass: add missing note-offs for all remaining open notes
    for (const auto& channelPair : openNotes) {
        int channel = channelPair.first;
        for (const auto& notePair : channelPair.second) {
            int noteNumber = notePair.first;

            // Create note-off at the end of the region
            auto* noteOff = new MidiMessage(MidiMessage::noteOff(channel, noteNumber, (juce::uint8)0));
            noteOff->setTimeStamp(closeTime);
            newMessages.push_back(noteOff);
        }
    }

    // Add all new note-off messages to the existing list
    for (auto* newMsg : newMessages) {
        midiMessageList.push_back(newMsg);
    }

    // Sort all messages by timestamp
    std::sort(midiMessageList.begin(), midiMessageList.end(),
        [](const MidiMessage* a, const MidiMessage* b) {
            if (!a) return true;
            if (!b) return false;
            return a->getTimeStamp() < b->getTimeStamp();
        });

    // Update region length if we added events beyond current length
    if (closeTime > numSamples) {
        setNumSamples(closeTime);
    }

    // Rebuild thumbnail and update display
    rebuildThumbnail();
    updateThumb();

    Logger::getCurrentLogger()->writeToLog(
        "MidiRegion::sanitize: Added " + String(newMessages.size()) + " missing note-off events"
    );
}