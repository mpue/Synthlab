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
	this->zoom = 20;
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

void MidiRegion::paint(Graphics& g) {

	if (dragger->isSelected(this)) {
		g.setColour(Colours::orange.brighter().brighter());
	}
	else {
		g.setColour(Colours::orange.brighter());
	}

	g.fillRoundedRectangle(getX(), getY(),getWidth(), getHeight(), 10);
	g.setColour(Colours::lightgrey);

	double audioLength = this->midiThumbnail->getTotalLength();
	this->midiThumbnail->drawChannels(g, 0.0, audioLength, 1.0f);

	g.setColour(Colours::steelblue.darker());
	g.drawRoundedRectangle(getX(), getY(), getWidth(), getHeight(), 10, 1.0f);

	g.setColour(Colours::darkblue);
	g.setFont(14.0);
	g.drawText(name, 10, 10, 140, 20, juce::Justification::left);

	if (loop) {
		for (int i = 1; i <= loopCount; i++) {
			g.setColour(Colours::steelblue);
			Rectangle<int> loopBounds(getX() + i * getWidth() / (loopCount + 1), getY(), getWidth(), getHeight());
			g.fillRoundedRectangle(getX() + i * getWidth() / (loopCount + 1), getY(),getWidth(), getHeight(), 10);
			g.setColour(Colours::grey);

			audioLength = this->midiThumbnail->getTotalLength();
			this->midiThumbnail->drawChannels(g, 0.0, audioLength, 1.0f);
		}
	}
}

void MidiRegion::resized() {
	if (this->midiThumbnail != nullptr) {
		Rectangle<int> bounds = this->midiThumbnail->getBounds().withHeight(getHeight());
		this->midiThumbnail->setBounds(bounds);
	}

	if (constrainer != nullptr) {
		constrainer->setMaxWidth(Project::getInstance()->getTrackLength()*sampleRate);
	}

	if (resizerR != NULL)
		resizerR->setTopLeftPosition(getWidth() - 5, 0);
		
}

void MidiRegion::setNumSamples(int numSamples) {
	this->numSamples = numSamples;
}

int MidiRegion::getNumSamples() {
	return numSamples;;
}

MidiBuffer* MidiRegion::getBuffer() {
	return midiBuffer;
}

MidiMessageSequence* MidiRegion::getSequence()
{
	return sequence;
}

MidiThumbnail* MidiRegion::getThumbnail() {
	return midiThumbnail;
}

void MidiRegion::setZoom(float zoom) {
	this->zoom = zoom;

	if (loop) {
		setSize(this->midiThumbnail->getTotalLength() * (loopCount + 1) * this->zoom, getHeight());
	}
	else {
		setSize(this->midiThumbnail->getTotalLength() * this->zoom, getHeight());
	}

	int newOffset = (this->sampleOffset / this->sampleRate) * zoom;
	this->setOffset(newOffset);
	this->setTopLeftPosition(newOffset, 0);

	this->midiThumbnail->setSize(this->midiThumbnail->getTotalLength() * this->zoom, getHeight());

	dragger->setRaster(this->zoom / 4);

	repaint();
}

void MidiRegion::setLoop(bool loop) {
	if (loop) {
		setSize(this->midiThumbnail->getTotalLength() * (loopCount + 1) * this->zoom, getHeight());
	}
	else {
		setSize(this->midiThumbnail->getTotalLength() * this->zoom, getHeight());
	}

	this->loop = loop;
	repaint();
}

// the timer is needed to updat the thumbnail during record
void MidiRegion::timerCallback() {
	updateThumb();
	timeElapsed += 100;
}

void MidiRegion::startRecording() {
	startTimer(100);
	currentMessageNum = 0;
}

void MidiRegion::setSampleOffset(long offset, bool reminder, bool notify)
{
	int delta = getSampleOffset() - offset;

	Region::setSampleOffset(offset, reminder, notify);

	//for each (auto message in midiMessageList)
	//{
	//	void setSampleOffset(long offset, bool reminder, bool notify);
	//	{
	//		message->setTimeStamp(message->getTimeStamp() - delta);
	//	}
	//}
}

void MidiRegion::stopRecording() {
	MidiMessage m = MidiMessage::allNotesOff(1);
	midiMessageList.push_back(&m);
	timeElapsed = 0;
	stopTimer();
	MidiMessage message;

	if (iterator == NULL) {
		iterator = new MidiBuffer::Iterator(*midiBuffer);
	}
	currentMessageNum = 0;
}

void MidiRegion::setDragger(MultiComponentDragger* dragger) {
	this->dragger = dragger;
}

void MidiRegion::updateThumb() {
	std::function<void(void)> changeLambda =
		[=]() {
		double length = this->getNumSamples() / sampleRate;
		setSize(length * this->zoom, getHeight());
		setBounds(0, 0, length * this->zoom, getHeight());
		Rectangle<int> bounds = this->midiThumbnail->getBounds().withWidth(length * this->zoom);
		this->midiThumbnail->setBounds(bounds);
		repaint();
		};
	juce::MessageManager::callAsync(changeLambda);
}

void MidiRegion::componentMovedOrResized(Component& component, bool wasMoved, bool wasResized) {
	/*
	if (wasMoved) {
		long tracklen = Project::getInstance()->getTrackLength();
		double sampleRate = Project::getInstance()->getSampleRate();

		long sampleNum = (tracklen / (tracklen * zoom)) * getBounds().getX() * getSampleRate();
		if (sampleNum != getSampleOffset()) {
			setSampleOffset(sampleNum, true, true);
		}

		if (getSampleOffset() >= tracklen * sampleRate) {

			setSampleOffset(tracklen * sampleRate - numSamples, false, false);
		}

		Logger::getCurrentLogger()->writeToLog("MidiRegion sampleOffset " + String(sampleOffset));
	}
	*/
	updateThumb();
}

void MidiRegion::changeListenerCallback(ChangeBroadcaster* source) {

}

void MidiRegion::clear() {
	midiMessages.clear();
	midiBuffer->clear();
	sequence->clear();
}

void MidiRegion::addMessage(MidiMessage* m, double time, int sampleNum) {
	Logger::getCurrentLogger()->writeToLog("Adding message at sample " + String(sampleNum) + " offset :" + String(getSampleOffset()));
	// midiMessages.insert(std::make_pair(timeElapsed,m));
	//sequence->addEvent(*m);

	// mSamples = sampleNum;
	// m->setTimeStamp(sampleNum); //  -getSampleOffset());
	// midiBuffer->addEvent(*m, sampleNum);

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

			if (abs(sampleNum - m->getTimeStamp()) <= 1 && m->getRawDataSize() < 5) {

				m = midiMessageList.at(i);

				// if (m->isNoteOn() || m->isNoteOff()) {
					// Logger::getCurrentLogger()->writeToLog("Getting message at sample " + String(sampleNum) + " " + m->getDescription());
					return m;
				// }


			}

		}
	}
	return nullptr;
}
