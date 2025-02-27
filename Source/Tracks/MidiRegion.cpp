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
    this->zoom = 30;
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
    this->zoom = 20;
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
    midiThumbnail->drawChannels(g, 0.0, getNumSamples() / sampleRate, 1.0f);
}

void MidiRegion::resized() {
    if (midiThumbnail != nullptr) {
        midiThumbnail->setBounds(getLocalBounds());
    }
    if (resizerR != nullptr) {
        resizerR->setBounds(getWidth() - 5, 0, 5, getHeight());
    }
}

void MidiRegion::updateThumb() {
    std::function<void(void)> changeLambda = [=]() {
        double lengthInSeconds = getNumSamples() / sampleRate;
        midiThumbnail->setBounds(sampleOffset / sampleRate * zoom, getY(), lengthInSeconds * zoom, getHeight());
        resized();
        repaint();
    };
    juce::MessageManager::callAsync(changeLambda);
}

void MidiRegion::componentMovedOrResized(Component& component, bool wasMoved, bool wasResized) {
    if (wasMoved) {
        float delta = (getBounds().getX() - lastX) / zoom;
        sampleOffset += delta * sampleRate;       
        Logger::getCurrentLogger()->writeToLog("Midi Region sample offset: " + String(sampleOffset));
        lastX = getBounds().getX();
        
        // Update the timestamp for all MIDI events when region is moved
        for (auto* m : midiMessageList) {
            if (m != nullptr) {
                // We don't modify the actual MidiMessage as it's const, but we update the thumbnail
                midiThumbnail->removeAllMessages();
                
                // Re-add all messages with updated positions
                for (auto* msg : midiMessageList) {
                    if (msg != nullptr) {
                        double timestamp = msg->getTimeStamp();
                        midiThumbnail->addMessage(timestamp - getSampleOffset(), msg);
                    }
                }
            }
        }
    }
    updateThumb();
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
    
    Logger::getCurrentLogger()->writeToLog("Adding message at sample " + String(sampleNum) + " offset :" + String(getSampleOffset()));
    midiMessageList.push_back(m);
    
    if (numSamples < sampleNum) {
		numSamples = sampleNum;
	}
   
    // Store message with timestamps relative to region start
    midiThumbnail->addMessage(sampleNum - getSampleOffset(), m);
    updateThumb();
}

MidiMessage* MidiRegion::getMessage(int sampleNum) {
    for (auto* m : midiMessageList) {
        if (m != nullptr && abs(sampleNum - m->getTimeStamp()) < 1 && m->getRawDataSize() < 5) {
            return m;
        }
    }
    return nullptr;
}

void MidiRegion::startRecording() {
    startTimer(100);
    currentMessageNum = 0;
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