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
    
    double length = this->midiThumbnail->getTotalLength();
    setSize(length * this->zoom, Project::DEFAULT_TRACK_HEIGHT);
    
    this->offset = 0;
    this->sequence = new MidiMessageSequence();
    this->constrainer = new ResizeConstrainer(1000000);
    
    resizerR = new ResizableEdgeComponent(this, constrainer, ResizableEdgeComponent::rightEdge);
    resizerR->setSize(5, getHeight());
    addAndMakeVisible(resizerR);
    resizerR->setTopLeftPosition(getWidth() - 5, 0);
    
    addComponentListener(this);
}

MidiRegion::MidiRegion(MidiBuffer* buffer, double sampleRate)
{
    this->midiBuffer = buffer;
    this->midiThumbnail = new MidiThumbnail(sampleRate);
    this->sampleRate = sampleRate;
    this->name = "copy";
    this->zoom = 20;
    double length = this->midiThumbnail->getTotalLength();
    setSize(length * this->zoom, Project::DEFAULT_TRACK_HEIGHT);
    this->offset = 0;
    this->sequence = new MidiMessageSequence();
    addComponentListener(this);
}


MidiRegion::~MidiRegion() {
    removeComponentListener(this);
    delete midiThumbnail;
    delete midiBuffer;
    delete sequence;
}

void MidiRegion::changeListenerCallback(ChangeBroadcaster* source) {
}


void MidiRegion::paint(Graphics& g) {
    if (dragger->isSelected(this)) {
        g.setColour(Colours::orange.brighter().brighter());
    } else {
        g.setColour(Colours::orange.brighter());
    }
    
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 10);
    g.setColour(Colours::lightgrey);
    
    double audioLength = this->midiThumbnail->getTotalLength();
    this->midiThumbnail->drawChannels(g, 0.0, audioLength, 1.0f);
}

void MidiRegion::resized() {
    if (this->midiThumbnail != nullptr) {
        this->midiThumbnail->setBounds(getLocalBounds());
    }
    if (resizerR != nullptr) {
        resizerR->setTopLeftPosition(getWidth() - 5, 0);
    }
}

void MidiRegion::updateThumb() {
    std::function<void(void)> changeLambda = [=]() {
        double length = this->getNumSamples() / sampleRate;
        setSize(length * this->zoom, getHeight());
        repaint();
    };
    juce::MessageManager::callAsync(changeLambda);
}

void MidiRegion::componentMovedOrResized(Component& component, bool wasMoved, bool wasResized) {
    if (wasMoved) {
        // calculate the delta in samples

        float delta = (getBounds().getX() - lastX) / zoom;
        Logger::getCurrentLogger()->getCurrentLogger()->writeToLog("Delta : " + String(delta));
        sampleOffset += delta * sampleRate;
        if (sampleOffset < 0) {
            sampleOffset = 0;
        }
        Logger::getCurrentLogger()->getCurrentLogger()->writeToLog("Sample offset: " + String(sampleOffset));
        lastX = getBounds().getX();

    }
    updateThumb();
}

void MidiRegion::setNumSamples(int numSamples) {
    this->numSamples = numSamples;
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
    setSize(this->midiThumbnail->getTotalLength() * this->zoom, getHeight());
    repaint();
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
    numSamples = sampleNum;
    this->midiThumbnail->addMessage(sampleNum - getSampleOffset(), m);
    updateThumb();
}

MidiMessage* MidiRegion::getMessage(int sampleNum) {

    MidiMessage* m = nullptr;

    for (int i = 0; i < midiMessageList.size(); i++) {

        if (midiMessageList.at(i) != NULL) {
            
            m = midiMessageList.at(i);

            if (abs(sampleNum - m->getTimeStamp()) < 1 && m->getRawDataSize() < 5) {
                m = midiMessageList.at(i);
                return m;
            }

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
    MidiMessage m = MidiMessage::allNotesOff(1);
    midiBuffer->addEvent(m, numSamples);
}

void MidiRegion::timerCallback() {
    updateThumb();
}
