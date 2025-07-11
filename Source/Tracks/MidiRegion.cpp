/*
  ==============================================================================

    MidiRegion.cpp
    Created: 28 Dec 2016 6:55:16pm
    Author:  Matthias Pueski

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

    double lengthInSeconds = numSamples / sampleRate;
    setBounds(sampleOffset / sampleRate * zoom, 0, lengthInSeconds * zoom, Project::DEFAULT_TRACK_HEIGHT);

    this->offset = 0;
    this->sequence = new MidiMessageSequence();

    this->constrainer = new ResizeConstrainer();
    constrainer->setMinimumWidth(20);
    constrainer->setMaximumWidth(1000000);

    resizerR = new ResizableEdgeComponent(this, constrainer, ResizableEdgeComponent::rightEdge);
    resizerR->setSize(5, getHeight());
    addAndMakeVisible(resizerR);

    addComponentListener(this);
}

MidiRegion::MidiRegion(MidiBuffer* buffer, double sampleRate) {
    this->midiBuffer = buffer;
    this->midiThumbnail = new MidiThumbnail(sampleRate);
    this->sampleRate = sampleRate;
    this->name = "copy";
    this->zoom = 1;
    this->numSamples = buffer->getNumEvents();

    double lengthInSeconds = numSamples / sampleRate;
    setBounds(0, 0, lengthInSeconds * zoom, Project::DEFAULT_TRACK_HEIGHT);

    this->offset = 0;
    this->sequence = new MidiMessageSequence();

    this->constrainer = new ResizeConstrainer();
    constrainer->setMinimumWidth(20);
    constrainer->setMaximumWidth(1000000);

    resizerR = new ResizableEdgeComponent(this, constrainer, ResizableEdgeComponent::rightEdge);
    resizerR->setSize(5, getHeight());
    addAndMakeVisible(resizerR);

    addComponentListener(this);
}

MidiRegion::~MidiRegion() {
    // delete midi messages

    for (auto* m : midiMessageList) {
		delete m;
	}

    removeComponentListener(this);
    delete midiThumbnail;
    delete midiBuffer;
    delete sequence;
}

void MidiRegion::paint(Graphics& g) {
    g.setColour(dragger->isSelected(this) ? Colours::orange.brighter().brighter() : Colours::orange.brighter());
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 10);
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
        long delta = newSampleOffset - sampleOffset;
        sampleOffset = newSampleOffset;

        Logger::getCurrentLogger()->writeToLog(
            "Moved! Bounds X: " + String(getBounds().getX()) +
            ", Zoom: " + String(zoom) +
            ", SampleRate: " + String(sampleRate) +
            ", New SampleOffset: " + String(newSampleOffset) +
            ", Delta: " + String(delta)
        );

        lastX = getBounds().getX();

		rebuildThumbnail();
        updateThumb();
    }
}

void MidiRegion::setNumSamples(int numSamples) {
    this->numSamples = numSamples;
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

void MidiRegion::setZoom(float zoom) {
    this->zoom = zoom;
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
    
    ScopedLock c(lock);

    // Logger::getCurrentLogger()->writeToLog("Adding message at sample " + String(sampleNum) + " offset :" + String(getSampleOffset()));
    
    m->setTimeStamp(sampleNum - getSampleOffset());

    midiMessageList.push_back(m);
    
    if (numSamples < sampleNum) {
		numSamples = sampleNum;
	}
   
    // Store message with timestamps relative to region start
    midiThumbnail->addMessage(sampleNum - getSampleOffset(), m);
    updateThumb();
}

MidiMessage* MidiRegion::getMessage(int sampleNum, int tolerance = 2)
{
    for (auto* m : midiMessageList)
    {
        if (m != nullptr &&
            std::abs(sampleNum - m->getTimeStamp()) <= tolerance &&
            m->isNoteOnOrOff()) // oder eine eigene sinnvollere Bedingung
        {
            Logger::getCurrentLogger()->writeToLog(
                "Found message at sample " + String(sampleNum) + ": " + m->getDescription());
            return m;
        }
    }
    return nullptr;
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

MidiRegion* MidiRegion::clone(long newOffset) {
    auto* copy = new MidiRegion(newOffset, this->numSamples, this->sampleRate, this->dragger);
    for (auto* msg : midiMessageList) {
        if (msg) {
            auto* clonedMsg = new MidiMessage(*msg);
            clonedMsg->setTimeStamp(clonedMsg->getTimeStamp());
            copy->addMessage(clonedMsg, 0, (int)(clonedMsg->getTimeStamp() + newOffset));
        }
    }
    copy->setZoom(this->zoom);
    copy->updateThumb();
    return copy;
}

std::pair<MidiRegion*, MidiRegion*> MidiRegion::splitAt(long splitSample, MultiComponentDragger* dragger) {
    long relativeSplit = splitSample - sampleOffset;
    if (relativeSplit <= 0 || relativeSplit >= numSamples)
        return { nullptr, nullptr };

    // Neue Regionen
    auto* left = new MidiRegion(sampleOffset, relativeSplit, sampleRate, dragger);
    auto* right = new MidiRegion(splitSample, numSamples - relativeSplit, sampleRate, dragger);

    // Note-On-Tracking für Pairing (nicht zwingend notwendig hier, aber für saubere Zukunft)
    std::map<int, MidiMessage*> openNotes;

    for (auto* msg : midiMessageList) {
        if (!msg) continue;
        int t = static_cast<int>(msg->getTimeStamp());

        // ----------- LEFT REGION -----------
        if (t < relativeSplit) {
            auto* copy = new MidiMessage(*msg);
            copy->setTimeStamp(t); // Timestamp relativ zur linken Region
            left->addMessage(copy, 0, t + sampleOffset);
        }

        // ----------- RIGHT REGION -----------
        else {
            int newRel = t - relativeSplit;
            auto* copy = new MidiMessage(*msg);
            copy->setTimeStamp(newRel); // Timestamp relativ zur rechten Region
            right->addMessage(copy, 0, newRel + splitSample);
        }
    }

    // Zoom und Längen setzen
    left->setZoom(zoom);
    right->setZoom(zoom);

    left->setNumSamples(relativeSplit);
    right->setNumSamples(numSamples - relativeSplit);

    left->rebuildThumbnail();
    right->rebuildThumbnail();

    left->updateThumb();
    right->updateThumb();

    return { left, right };
}

void MidiRegion::quantize(int quantizeGridSamples)
{
    const ScopedLock sl(lock);

    std::map<int, MidiMessage*> noteOns;
    std::vector<MidiMessage*> quantizedMessages;

    for (auto* msg : midiMessageList)
    {
        if (!msg)
            continue;

        if (msg->isNoteOn())
        {
            int note = msg->getNoteNumber();
            int originalTime = (int)msg->getTimeStamp();
            int quantizedTime = ((originalTime + quantizeGridSamples / 2) / quantizeGridSamples) * quantizeGridSamples;

            auto* quantized = new MidiMessage(*msg);
            quantized->setTimeStamp(quantizedTime);
            quantizedMessages.push_back(quantized);

            noteOns[note] = quantized;
        }
        else if (msg->isNoteOff())
        {
            int note = msg->getNoteNumber();
            int originalTime = (int)msg->getTimeStamp();

            auto it = noteOns.find(note);
            if (it != noteOns.end())
            {
                int noteOnTime = (int)it->second->getTimeStamp();
                int duration = originalTime - (int)msg->getTimeStamp() + noteOnTime;

                auto* quantized = new MidiMessage(*msg);
                quantized->setTimeStamp(noteOnTime + duration);
                quantizedMessages.push_back(quantized);
            }
            else
            {
                auto* quantized = new MidiMessage(*msg);
                quantized->setTimeStamp(((originalTime + quantizeGridSamples / 2) / quantizeGridSamples) * quantizeGridSamples);
                quantizedMessages.push_back(quantized);
            }
        }
        else
        {
            // andere Messages übernehmen
            auto* quantized = new MidiMessage(*msg);
            quantized->setTimeStamp(((int)msg->getTimeStamp() + quantizeGridSamples / 2) / quantizeGridSamples * quantizeGridSamples);
            quantizedMessages.push_back(quantized);
        }
    }

    // Alte löschen
    for (auto* msg : midiMessageList)
        delete msg;

    midiMessageList = quantizedMessages;

    rebuildThumbnail();
    updateThumb();
}


