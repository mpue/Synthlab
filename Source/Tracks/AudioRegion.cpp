/*
  ==============================================================================

    Track.cpp
    Created: 25 Nov 2016 10:00:36pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "AudioRegion.h"
#include "../Project/Project.h"

AudioRegion::AudioRegion(AudioRegion* other, AudioFormatManager& manager, double sampleRate, long startSample, long numSamples) {

    this->sampleRate = sampleRate;
    this->zoom = other->zoom;
    this->constrainer = new ResizeConstrainer(this->zoom / 4);
    
    resizerR = new ResizableEdgeComponent(this,constrainer,ResizableEdgeComponent::rightEdge);
    // resizerL = new ResizableEdgeComponent(this,constrainer,ResizableEdgeComponent::leftEdge);
    
    this->thumbnailCache = new AudioThumbnailCache(1);
    this->thumbnail = new AudioThumbnail(512, manager, *this->thumbnailCache);
    this->thumbnail->addChangeListener(this);
    
    audioBuffer = new AudioSampleBuffer(2, numSamples);
    audioBuffer->copyFrom(0, 0, *other->getBuffer(), 0, startSample, numSamples);
    audioBuffer->copyFrom(1, 0, *other->getBuffer(), 1, startSample, numSamples);
    
    this->thumbnail->reset(2, sampleRate);
    this->thumbnail->addBlock(0, *audioBuffer, 0,numSamples);
    
    this->name = other->getName();
    
    /*
    double length = this->thumbnail->getTotalLength();
    setSize(length * this->zoom, other->getHeight());
    */
    
    setSize((other->getBuffer()->getNumSamples() / sampleRate) * this->zoom, Project::DEFAULT_TRACK_HEIGHT-25);

    this->offset = 0;
    
    //addAndMakeVisible(resizerL);
    addAndMakeVisible(resizerR);
    
    addComponentListener(this);
    dirty = false;
    setPitch(1);
    createProperties();
}


AudioRegion::AudioRegion(AudioSampleBuffer* source, AudioFormatManager& manager, long startSample, long sampleLength, double sampleRate) {

    this->zoom = Project::getInstance()->getTempo() / 8;
    this->constrainer = new ResizeConstrainer(this->zoom / 4);
    
    resizerR = new ResizableEdgeComponent(this,constrainer,ResizableEdgeComponent::rightEdge);
    // resizerL = new ResizableEdgeComponent(this,constrainer,ResizableEdgeComponent::leftEdge);
    
    this->sampleRate = sampleRate;
    
    this->thumbnailCache = new AudioThumbnailCache(1);
    this->thumbnail = new AudioThumbnail(512, manager, *this->thumbnailCache);
    this->thumbnail->addChangeListener(this);
    
    audioBuffer = new AudioSampleBuffer(2, sampleLength);
    
    if (source != nullptr) {
        audioBuffer->copyFrom(0, 0, *source, 0, 0, sampleLength);
        audioBuffer->copyFrom(1, 0, *source, 1, 0, sampleLength);
    }
    
    this->thumbnail->reset(2, sampleRate);
    this->thumbnail->addBlock(0, *audioBuffer, 0, sampleLength);
    
    this->name = "new";
    
    /*
    double length = this->thumbnail->getTotalLength();
    setSize(length * this->zoom, Project::DEFAULT_TRACK_HEIGHT);
    */
    
    setSize((audioBuffer->getNumSamples() / sampleRate) * this->zoom, Project::DEFAULT_TRACK_HEIGHT);

    this->offset = 0;
    
    // addAndMakeVisible(resizerL);
    addAndMakeVisible(resizerR);
    
    addComponentListener(this);
    
    dirty = false;
    // setPitch(1);
    createProperties();
}

AudioRegion::AudioRegion(AudioRegion* other, AudioFormatManager& manager, double sampleRate) {
    
    this->zoom = other->zoom;
    this->constrainer = new ResizeConstrainer(this->zoom / 4);
    
    resizerR = new ResizableEdgeComponent(this,constrainer,ResizableEdgeComponent::rightEdge);
    //resizerL = new ResizableEdgeComponent(this,constrainer,ResizableEdgeComponent::leftEdge);
    
    this->sampleRate = sampleRate;
    
    this->thumbnailCache = new AudioThumbnailCache(1);
    this->thumbnail = new AudioThumbnail(512, manager, *this->thumbnailCache);
    this->thumbnail->addChangeListener(this);
    this->setThumbnailBounds(other->getThumbnailBounds());
    
    audioBuffer = new AudioSampleBuffer(2, other->getBuffer()->getNumSamples());
    audioBuffer->copyFrom(0, 0, *other->getBuffer(), 0, 0, other->getBuffer()->getNumSamples());
    audioBuffer->copyFrom(1, 0, *other->getBuffer(), 1, 0, other->getBuffer()->getNumSamples());
    
    this->thumbnail->reset(2, sampleRate);
    this->thumbnail->addBlock(0, *audioBuffer, 0,other->getBuffer()->getNumSamples());
    
    this->name = other->getName();
    
    /*
    double length = this->thumbnail->getTotalLength();
    setSize(length * this->zoom, other->getHeight());
     */
    
    setSize((other->getBuffer()->getNumSamples() / sampleRate) * this->zoom, Project::DEFAULT_TRACK_HEIGHT);

    this->offset = 0;
    
    //addAndMakeVisible(resizerL);
    addAndMakeVisible(resizerR);
   
    addComponentListener(this);
    
    dirty = false;

    setPitch(1);
    createProperties();
}

//==============================================================================
AudioRegion::AudioRegion(File file, String refId, AudioFormatManager& manager, double sampleRate)
{
    this->zoom = Project::getInstance()->getTempo() / 4;
    this->constrainer = new ResizeConstrainer(this->zoom / 4);

    resizerR = new ResizableEdgeComponent(this,constrainer,ResizableEdgeComponent::rightEdge);
    //resizerL = new ResizableEdgeComponent(this,constrainer,ResizableEdgeComponent::leftEdge);

	this->sampleRate = sampleRate;

    AudioFormatReader* reader = manager.createReaderFor(file);
    ScopedPointer<AudioFormatReaderSource> afr = new AudioFormatReaderSource(reader, true);

    this->thumbnailCache = new AudioThumbnailCache(1);
    this->thumbnail = new AudioThumbnail(512, manager, *this->thumbnailCache);
    this->thumbnail->addChangeListener(this);
    
    audioBuffer = new AudioSampleBuffer(2, reader->lengthInSamples);
    reader->read(audioBuffer, 0, reader->lengthInSamples, 0, true, true);

    this->thumbnail->reset(2, sampleRate);
    this->thumbnail->addBlock(0, *audioBuffer, 0,reader->lengthInSamples);
     
    this->name = file.getFileNameWithoutExtension();
    this->clipRefId = refId;

    setSize((audioBuffer->getNumSamples() / sampleRate) * this->zoom, Project::DEFAULT_TRACK_HEIGHT);
    //  Logger::getCurrentLogger()->writeToLog(getThumbnailBounds()->toString());
    Logger::getCurrentLogger()->writeToLog(getBounds().toString());
    
	this->offset = 0;
    
    addComponentListener(this);
    
    // addAndMakeVisible(resizerL);
    addAndMakeVisible(resizerR);
    
    dirty = false;
    setPitch(1);
    createProperties();
}

AudioRegion::~AudioRegion()
{
    delete this->thumbnailCache;
    delete this->thumbnail;
    delete this->audioBuffer;
    delete this->audioBufferProcessed;
    // delete this->resizerL;
    delete this->resizerR;
    delete this->constrainer;
    delete this->stretcher;   
}

void AudioRegion::timerCallback() {
    if (dirty)
        updateThumb();
}

void AudioRegion::updateThumb()
{
    // double length = this->thumbnail->getTotalLength();
    this->thumbnail->reset(2, sampleRate);
    this->thumbnail->addBlock(0, *audioBuffer, 0, audioBuffer->getNumSamples());
    double length = thumbnail->getTotalLength();
    setSize(length * this->zoom, getHeight());
    this->thumbnailBounds->setWidth(length * this->zoom);
    repaint();
    dirty = false;
}


void AudioRegion::setDragger(MultiComponentDragger *dragger) {
    this->dragger = dragger;
}

void AudioRegion::startRecording() {
    getThumbnail()->reset(2, Project::getInstance()->getSampleRate());
    startTimer(100);
}

void AudioRegion::stopRecording() {
    stopTimer();
}

void AudioRegion::setLoop(bool loop)
{
    if (loop) {
         setSize(this->thumbnail->getTotalLength() * (loopCount + 1) * this->zoom, getHeight());
    }
    else {
         setSize(this->thumbnail->getTotalLength() * this->zoom, getHeight());
    }
    
    this->loop = loop;
    dirty = true;
}

const float* AudioRegion::getReadBuffer(int channel) {
    return audioBuffer->getReadPointer(channel);
}

AudioSampleBuffer* AudioRegion::getBuffer() {
    return audioBuffer;
}

void AudioRegion::setZoom(float zoom) {
    this->zoom = zoom;
    
    if (loop) {
        // setSize(this->thumbnail->getTotalLength() * (loopCount + 1) * this->zoom, getHeight());
        setSize((getNumSamples() / sampleRate * (loopCount + 1)) * this->zoom, getHeight());

    }
    else {
        // setSize(this->thumbnail->getTotalLength() * this->zoom, getHeight());
        // setSize(getNumSamples() / sampleRate * this->zoom, getHeight());
        setSize((getNumSamples() / sampleRate) * this->zoom, getHeight());

    }
    
    int newOffset = (this->sampleOffset / this->sampleRate) * zoom;
    this->setOffset(newOffset);

    this->setTopLeftPosition(newOffset, 0);
    
    resizerR->setSize(5, getHeight());
    resizerR->setTopLeftPosition(getWidth() - 5, 0);
    
    // resizerL->setSize(5, getHeight());
    // resizerL->setTopLeftPosition(0, 0);
    
    // setBounds(0,0,this->thumbnail->getTotalLength() * this->zoom, 200);
    this->thumbnailBounds->setSize(getWidth(), getHeight());
    // this->thumbnailBounds->setBounds(getBounds().getX(),getBounds().getY(),getWidth(),getHeight());

    dragger->setRaster(this->zoom / 4);
    constrainer->setRaster(this->zoom / 4);
    constrainer->setMaxWidth((audioBuffer->getNumSamples() / sampleRate) * this->zoom);
    
    dirty = true;
}

void AudioRegion::setThumbnailBounds(Rectangle<int>* bounds) {
    this->thumbnailBounds = bounds;
}

Rectangle<int>*  AudioRegion::getThumbnailBounds() {
    return new Rectangle<int>(*this->thumbnailBounds);
};

void AudioRegion::paint (Graphics& g)
{  
    Rectangle<int> b = *this->thumbnailBounds;
    int effectiveWidth = getWidth() ;//(this->getNumSamples() / sampleRate) * this->zoom;
    
	if (dragger->isSelected(this)) {
		g.setColour(Colours::steelblue.brighter());
	}
	else {
		g.setColour(Colours::steelblue);
	}

    g.fillRoundedRectangle(b.getX(),b.getY(),effectiveWidth,b.getHeight(),10);
    g.setColour(Colours::white);
    
    const double audioLength(this->thumbnail->getTotalLength());
    this->thumbnail->drawChannels(g, b, 0.0, audioLength, 1.0f);
    
    g.setColour(Colours::steelblue.darker());    
    g.drawRoundedRectangle(b.getX(),b.getY(),effectiveWidth,b.getHeight(),10,1.0f);
    
    g.setColour(Colours::darkblue);
    g.setFont(14.0);
    g.drawText(name, 10, 10, 140, 20, juce::Justification::left);
    g.drawText(juce::String(sampleOffset), 10, 30, 140, 20, juce::Justification::left);

    if(loop) {
        for (int i = 1; i <= loopCount;i++) {
            g.setColour(Colours::steelblue);
            Rectangle<int> loopBounds(b.getX() + i * effectiveWidth / (loopCount + 1),b.getY(),effectiveWidth,b.getHeight());
            g.fillRoundedRectangle(b.getX() + i * effectiveWidth / (loopCount + 1) ,b.getY(),effectiveWidth,b.getHeight(),10);
            g.setColour(Colours::grey);

            const double audioLength(this->thumbnail->getTotalLength());
            this->thumbnail->drawChannels(g, loopBounds,0.0,audioLength, 1.0f);
        }
    }
    dirty = false;
}

void AudioRegion::resized()
{
    Logger::getCurrentLogger()->writeToLog(String(this->thumbnail->getTotalLength()));
    if (this->thumbnailBounds != NULL) {
        this->thumbnailBounds->setHeight(getHeight());
        // this->thumbnailBounds->setWidth(getWidth());
    }
    
    constrainer->setRaster(zoom / 4);
    int maxWidth = (audioBuffer->getNumSamples() / sampleRate) * this->zoom;
    constrainer->setMaxWidth(maxWidth);
    resizerR->setTopLeftPosition(getWidth() - 5, 0);
    dirty = true;
}

AudioThumbnail* AudioRegion::getThumbnail() {
    
    return this->thumbnail;
}

void AudioRegion::changeListenerCallback(ChangeBroadcaster * source)
{
    if (source == this->thumbnail) {
        dirty = true;
        
    }
}

int AudioRegion::getNumSamples() {
    
    int originalSize = thumbnailBounds->getWidth();
    int reducedSize = getWidth();
    
    //cout << "Original : " << originalSize << " reduced : " << reducedSize << endl;
    
    // region has been resized with resizer component, thus we need to report another sample size
    if (reducedSize > 0 && reducedSize  < originalSize) {
        int effectiveSamples = (getWidth() / zoom) * sampleRate;
        
        // std::cout << effectiveSamples << std::endl;
        return effectiveSamples;
    }
    
    
    return audioBuffer->getNumSamples();
}

void AudioRegion::componentMovedOrResized (Component& component, bool wasMoved, bool wasResized) {
    
    if (wasMoved) {
        long tracklen = Project::getInstance()->getTrackLength();
        double sampleRate = 48000; // Project::getInstance()->getSampleRate();
        
        long sampleNum = (tracklen / (tracklen * zoom)) * getBounds().getX() * getSampleRate();
        
        oldOffset = sampleNum;
        
        if (sampleNum != getSampleOffset()) {
            setSampleOffset(sampleNum, true, true);
        }
        
        if (getSampleOffset() >= tracklen * sampleRate) {
            
            setSampleOffset(tracklen * sampleRate - getBuffer()->getNumSamples(), false, false);
        }
        
        Logger::getCurrentLogger()->writeToLog("AudioRegion sampleOffset " + String(sampleOffset));
    }
    if (wasResized) {
        // track needs to be notified about resizing in order to adjust the sample buffer
        sendChangeMessage();
    }
    
    dirty = true;
}

void AudioRegion::setClipRefId(String id) {
    this->clipRefId = id;
}

String AudioRegion::getClipRefid() {
    return clipRefId;
}

float AudioRegion::getPitch() {
    return pitch;
}


void AudioRegion::setPitch(float pitch) {

    if (pitch < 0.1) {
        pitch = 0.1;
    }
    if (pitch > 2) {
        pitch = 2;
    }

    this->pitch = pitch;

    if (stretcher == nullptr) {
        stretcher = new RubberBand::RubberBandStretcher(48000, 2, RubberBand::RubberBandStretcher::OptionProcessRealTime);
    }
    stretcher->setPitchScale(this->pitch);
    // stretcher->setTimeRatio(this->pitch);

    stretcher->reset();
    stretcher->process(audioBuffer->getArrayOfReadPointers(), audioBuffer->getNumSamples(), true);
    stretcher->retrieve(audioBuffer->getArrayOfWritePointers(), audioBuffer->getNumSamples());
    
    this->thumbnail->reset(2, sampleRate);
    this->thumbnail->addBlock(0, *audioBuffer, 0, audioBuffer->getNumSamples());
    // updateThumb();

    /*
    if (audioBuffer != nullptr) 
    {
        if (this->tempBufferLeft == nullptr)
            this->tempBufferLeft = new float[audioBuffer->getNumSamples() * 2];
        if (this->tempBufferRight == nullptr)
            this->tempBufferRight = new float[audioBuffer->getNumSamples() * 2];
        interpolatorLeft->reset();
        interpolatorRight->reset();
        interpolatorLeft->process(this->pitch, audioBuffer->getReadPointer(0), tempBufferLeft, audioBuffer->getNumSamples()/this->pitch);
        interpolatorRight->process(this->pitch, audioBuffer->getReadPointer(1), tempBufferRight, audioBuffer->getNumSamples() / this->pitch);
        audioBufferProcessed = new AudioSampleBuffer(2,audioBuffer->getNumSamples()*2);
        audioBufferProcessed->copyFrom(0, 0, tempBufferLeft, audioBuffer->getNumSamples() * 2);
        audioBufferProcessed->copyFrom(1, 0, tempBufferRight, audioBuffer->getNumSamples() * 2);
    }
    */
}



juce::Array<juce::PropertyComponent*>& AudioRegion::getProperties()
{
    juce::Array<juce::PropertyComponent*> properties = Region::getProperties();

    properties = Array<PropertyComponent*>();

    pitchProp = new SliderPropertyComponent(*pitchValue, "Pitch", 0.1, 2.0f, 0.1, 1, true);
    properties.add(pitchProp);
    return properties;
}

void AudioRegion::createProperties()
{
    Region::createProperties();
    pitchValue = new Value();
    pitchListener = new PitchListener(*pitchValue, this);
    pitchValue->setValue(pitch);
}

void AudioRegion::resizeAudio(int length)
{
    AudioSampleBuffer* temp = new AudioSampleBuffer(2, audioBuffer->getNumSamples() + length);
    temp->copyFrom(0, 0, audioBuffer->getReadPointer(0), audioBuffer->getNumSamples());
    temp->copyFrom(1, 0, audioBuffer->getReadPointer(1), audioBuffer->getNumSamples());

    AudioSampleBuffer* tempToBeDeleted = audioBuffer;

    audioBuffer->clear();
    audioBuffer = temp;    
}
