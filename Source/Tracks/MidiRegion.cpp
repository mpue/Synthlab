/*
  ==============================================================================

    MidiRegion.cpp
    Created: 28 Dec 2016 6:55:16pm
    Author:  Matthias Pueski
    Modified: Mit globalem Tempo-Support

  ==============================================================================
*/

#include "MidiRegion.h"

MidiRegion::MidiRegion(long startSample, long sampleLength, double sampleRate, MultiComponentDragger* dragger) {
    this->dragger = dragger;
    this->midiBuffer = new MidiBuffer();
    this->midiThumbnail = new MidiThumbnail(sampleRate);
    this->sampleRate = sampleRate;
    this->sampleOffset = startSample;
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
    resizerR->setSize(5, getHeight());
    addAndMakeVisible(resizerR);

    addComponentListener(this);

    auto& bus = FastBus::getInstance();

    bus.subscribe<float>("tempo", [this](const FastTopic<float>& topic) {
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
    resizerR->setSize(5, getHeight());
    addAndMakeVisible(resizerR);

    addComponentListener(this);

    auto& bus = FastBus::getInstance();

    bus.subscribe<float>("tempo", [this](const FastTopic<float>& topic) {
        handleTempoChange(topic.getValue());
    });

}

MidiRegion::~MidiRegion() {

    auto& bus = FastBus::getInstance();
	bus.removeTopic<float>("tempo");

    removeAllChangeListeners();

    // delete midi messages
    for (auto* m : midiMessageList) {
        delete m;
    }

    removeComponentListener(this);
    delete midiThumbnail;
    delete midiBuffer;
    delete sequence;
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
    g.setColour(dragger->isSelected(this) ? Colours::orange.brighter().brighter() : Colours::orange.brighter());
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 10);

    // BPM-Info anzeigen
    g.setColour(Colours::white);
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

    g.setColour(Colours::lightgrey);
    double visibleTime = (getNumSamples() / sampleRate);
    midiThumbnail->drawChannels(g, 0.0, visibleTime, 1.0f);
}

void MidiRegion::resized() {
    if (midiThumbnail != nullptr) {
        int x = (int)((sampleOffset / sampleRate) * zoom);
        int w = (int)((getNumSamples() / sampleRate) * zoom);
        midiThumbnail->setBounds(x, getY(), w, getHeight());
        midiThumbnail->setZoom(zoom);
    }
    if (resizerR != nullptr) {
        resizerR->setBounds(getWidth() - 5, 0, 5, getHeight());
    }
}

void MidiRegion::updateThumb() {
    std::function<void(void)> changeLambda = [=]() {
        int x = (int)((sampleOffset / sampleRate) * zoom);
        int w = (int)((getNumSamples() / sampleRate) * zoom);
        midiThumbnail->setBounds(x, getY(), w, getHeight());
        midiThumbnail->setZoom(zoom);
        midiThumbnail->repaint();

        resized();
        repaint();
        };
    juce::MessageManager::callAsync(changeLambda);
}

void MidiRegion::componentMovedOrResized(Component& component, bool wasMoved, bool wasResized) {
    if (wasMoved) {
        long newSampleOffset = std::round((getBounds().getX() / zoom) * sampleRate);

        // Optional: An Grid snappen
        if (snapToGrid) {
            newSampleOffset = snapSampleToGrid(newSampleOffset) - sampleOffset + newSampleOffset;
        }

        long delta = newSampleOffset - sampleOffset;
        sampleOffset = newSampleOffset;

        Logger::getCurrentLogger()->writeToLog("Region moved - synchronizing bounds with sampleOffset");

        // WICHTIG: Bounds mit neuem Offset synchronisieren
        double lengthInSeconds = numSamples / sampleRate;
        setBounds((int)(sampleOffset / sampleRate * zoom), getY(),
            (int)(lengthInSeconds * zoom), getHeight());

        rebuildThumbnail();
        updateThumb();
    }
}

void MidiRegion::setNumSamples(int numSamples) {
    this->numSamples = numSamples;
    setBounds(getX(), getY(), numSamples / sampleRate * zoom, getHeight());
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
    setBounds((int)(sampleOffset / sampleRate * zoom), getY(),
        (int)(lengthInSeconds * zoom), getHeight());

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

void MidiRegion::addMessage(MidiMessage* m, double time, int sampleNum) {

    sampleNum -= sampleNum % Project::getInstance()->getBufferSize();

    ScopedLock c(lock);

    // Optional: Message an Grid snappen
    if (snapToGrid) {
        int relativeSample = sampleNum - getSampleOffset();
        int snappedSample = snapSampleToGrid(relativeSample + getSampleOffset()) - getSampleOffset();
        sampleNum = snappedSample + getSampleOffset();
    }

    m->setTimeStamp(sampleNum - getSampleOffset());

    midiMessageList.push_back(m);

    int relativeSample = sampleNum - getSampleOffset();
    if (relativeSample > numSamples) {
        setNumSamples(relativeSample);
    }

    midiThumbnail->addMessage(sampleNum - getSampleOffset(), m);
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
    vector<MidiMessage*> foundMessages;
    const int endSample = sampleNum + numSamples;

    for (auto* m : midiMessageList)
    {
        if (m != nullptr &&
            m->getTimeStamp() >= sampleNum &&
            m->getTimeStamp() < endSample &&
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

    // Tempo-Einstellungen übertragen
    left->setTempo(currentBPM);
    left->setQuantizeGrid(quantizeGridBeats);
    left->setSnapToGrid(snapToGrid);

    right->setTempo(currentBPM);
    right->setQuantizeGrid(quantizeGridBeats);
    right->setSnapToGrid(snapToGrid);

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

MidiMessageSequence* MidiRegion::getMidiMessageSequence()
{
    MidiMessageSequence* seq = new MidiMessageSequence();
    for (auto* msg : midiMessageList)
    {
        if (msg != nullptr) {
            seq->addEvent(*msg);
        }
    }
    Logger::getCurrentLogger()->writeToLog("MidiRegion::getMidiMessageSequence: " + String(seq->getNumEvents()) + " events");

    seq->updateMatchedPairs();

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

    // Skaliere alle MIDI-Events entsprechend
    if (oldBPM > 0 && this->currentBPM > 0) {
        rescaleMidiEvents(oldBPM, this->currentBPM);
    }
	updateBoundsFromTempo();
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