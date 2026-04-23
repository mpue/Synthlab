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
#include "../AudioEngine/SmartSample.h"

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

    if (other->getBuffer()->getNumChannels() == 1) {
        audioBuffer->copyFrom(0, 0, *other->getBuffer(), 0, startSample, numSamples);
        audioBuffer->copyFrom(1, 0, *other->getBuffer(), 0, startSample, numSamples);
    }
    else {
        audioBuffer->copyFrom(0, 0, *other->getBuffer(), 0, startSample, numSamples);
        audioBuffer->copyFrom(1, 0, *other->getBuffer(), 1, startSample, numSamples);
    }
        
    this->thumbnail->reset(2, sampleRate);
    this->thumbnail->addBlock(0, *audioBuffer, 0, numSamples);
    
    this->name = other->getName();
    
    // KORREKTUR: Verwende numSamples statt other->getBuffer()->getNumSamples()
    // da wir nur einen Teil des Buffers kopiert haben
    setSize((numSamples / sampleRate) * this->zoom, Project::DEFAULT_TRACK_HEIGHT - 25);

    this->offset = 0;
    
    //addAndMakeVisible(resizerL);
    addAndMakeVisible(resizerR);
    
    addComponentListener(this);
    dirty = false;
    setPitch(1);
    createProperties();
    updateSnapSettings();
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

        if (source->getNumChannels() == 1) {
            audioBuffer->copyFrom(0, 0, *source, 0, 0, sampleLength);
            audioBuffer->copyFrom(1, 0, *source, 0, 0, sampleLength);
        }
        else {
            audioBuffer->copyFrom(0, 0, *source, 0, 0, sampleLength);
            audioBuffer->copyFrom(1, 0, *source, 1, 0, sampleLength);
        }
    }
    
    this->thumbnail->reset(2, sampleRate);
    this->thumbnail->addBlock(0, *audioBuffer, 0, sampleLength);
    
    this->name = "new";
    this->offset = 0;
        
    setSize((audioBuffer->getNumSamples() / sampleRate) * this->zoom, Project::DEFAULT_TRACK_HEIGHT);
    
    // addAndMakeVisible(resizerL);
    addAndMakeVisible(resizerR);    
    addComponentListener(this);
    
    dirty = false;
    // setPitch(1);
    createProperties();
    updateSnapSettings();
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

    if (other->getBuffer()->getNumChannels() == 1) {
        audioBuffer->copyFrom(0, 0, *other->getBuffer(), 0, 0, other->getBuffer()->getNumSamples());
        audioBuffer->copyFrom(1, 0, *other->getBuffer(), 0, 0, other->getBuffer()->getNumSamples());
    }
    else {
        audioBuffer->copyFrom(0, 0, *other->getBuffer(), 0, 0, other->getBuffer()->getNumSamples());
        audioBuffer->copyFrom(1, 0, *other->getBuffer(), 1, 0, other->getBuffer()->getNumSamples());
    }
        
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
    updateSnapSettings();
}

//==============================================================================
AudioRegion::AudioRegion(File file, String refId, AudioFormatManager& manager, double sampleRate)
{
    this->zoom = 40;
    this->constrainer = new ResizeConstrainer(this->zoom / 4);
    resizerR = new ResizableEdgeComponent(this, constrainer, ResizableEdgeComponent::rightEdge);
    this->sampleRate = sampleRate;

    SmartSample* smartSample = new SmartSample();
    smartSample->loadFromFile(file, sampleRate);

    int numChannels = smartSample->getBuffer().getNumChannels();
    int lengthInSamples = smartSample->getLengthInSamples();

    this->thumbnailCache = new AudioThumbnailCache(5);
    this->thumbnail = new AudioThumbnail(512, manager, *this->thumbnailCache);
    this->thumbnail->addChangeListener(this);
    this->thumbnail->reset(numChannels, sampleRate);

    audioBuffer = new AudioSampleBuffer(smartSample->getBuffer());

    // KORRIGIERT: addBlock mit korrektem Buffer-Offset (immer 0, da wir den ganzen Buffer �bergeben)
    this->thumbnail->addBlock(0, *audioBuffer, 0, audioBuffer->getNumSamples());

    this->name = file.getFileNameWithoutExtension();
    this->clipRefId = refId;
    setSize((lengthInSamples / sampleRate) * this->zoom, Project::DEFAULT_TRACK_HEIGHT);

    Logger::getCurrentLogger()->writeToLog(getBounds().toString());

    this->offset = 0;

    addComponentListener(this);
    addAndMakeVisible(resizerR);

    dirty = false;
    setPitch(1);
    createProperties();
    updateSnapSettings();

    // setInterceptsMouseClicks(false, true);

    delete smartSample;
}

AudioRegion::~AudioRegion()
{
    if (pitchValue)
        pitchValue->removeListener(pitchListener);
    delete pitchListener;
    delete pitchValue;

    delete[] tempBufferLeft;
    delete[] tempBufferRight;

    delete this->thumbnailBounds;
    delete this->thumbnailCache;
    delete this->thumbnail;
    delete this->audioBuffer;
    delete this->audioBufferProcessed;
    // resizerR and resizerL are deleted by Region::~Region()
    delete this->constrainer;
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

void AudioRegion::consolidate()
{
    int numChannels = audioBuffer->getNumChannels();

    if (loop && loopCount > 0)
    {
        // Looping case: repeat the audio buffer (loopCount + 1) times.
        // Call updateThumb() afterwards so the component expands to the full length.
        int oneLoopSamples = audioBuffer->getNumSamples();
        int totalSamples   = oneLoopSamples * (loopCount + 1);

        auto* newBuffer = new AudioSampleBuffer(numChannels, totalSamples);

        for (int i = 0; i <= loopCount; i++)
            for (int ch = 0; ch < numChannels; ch++)
                newBuffer->copyFrom(ch, i * oneLoopSamples, *audioBuffer, ch, 0, oneLoopSamples);

        delete audioBuffer;
        audioBuffer = newBuffer;

        loopCount = 0;
        loop      = false;
        dirty     = true;
        updateThumb();
    }
    else
    {
        // Non-looping case: trim the audio buffer to the current visual (dragged) length.
        // Use the component width directly — independent of thumbnailBounds state.
        int trimmedSamples = (int)((getWidth() / zoom) * sampleRate);
        if (trimmedSamples > 0 && trimmedSamples < audioBuffer->getNumSamples())
        {
            auto* newBuffer = new AudioSampleBuffer(numChannels, trimmedSamples);
            for (int ch = 0; ch < numChannels; ch++)
                newBuffer->copyFrom(ch, 0, *audioBuffer, ch, 0, trimmedSamples);
            delete audioBuffer;
            audioBuffer = newBuffer;

            // Rebuild thumbnail for the new (shorter) buffer.
            this->thumbnail->reset(numChannels, sampleRate);
            this->thumbnail->addBlock(0, *audioBuffer, 0, trimmedSamples);
            // Align thumbnailBounds so getNumSamples() returns the buffer size going forward.
            this->thumbnailBounds->setWidth(getWidth());
            dirty = true;
            repaint();
        }
    }
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
        setSize((getNumSamples() / sampleRate * (loopCount + 1)) * this->zoom, getHeight());
    }
    else {
        setSize((getNumSamples() / sampleRate) * this->zoom, getHeight());
    }

    // Snap auf Viertelnoten anwenden
    long snappedOffset = snapToQuarterNote(this->sampleOffset);
    int newOffset = (snappedOffset / this->sampleRate) * zoom;
    this->setOffset(newOffset);
    this->setTopLeftPosition(newOffset, 0);

    resizerR->setSize(5, getHeight());
    resizerR->setTopLeftPosition(getWidth() - 5, 0);

    this->thumbnailBounds->setSize(getWidth(), getHeight());

    // Aktualisiere Snap-Einstellungen f�r Viertelnoten
    updateSnapSettings();

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
    
    double audioLength = this->thumbnail->getTotalLength();
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

            audioLength = this->thumbnail->getTotalLength();
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

void AudioRegion::componentMovedOrResized(Component& component, bool wasMoved, bool wasResized) {

    if (wasMoved) {
        long tracklen = Project::getInstance()->getTrackLength();
        double sampleRate = Project::getInstance()->getSampleRate();

        // Berechne Sample-Position basierend auf X-Position
        long sampleNum = (getBounds().getX() / zoom) * sampleRate;

        // Snap auf Viertelnote
        sampleNum = snapToQuarterNote(sampleNum);

        // Gerade Sample-Zahl sicherstellen
        if (sampleNum % 2 != 0)
            sampleNum -= 1;

        oldOffset = sampleNum;

        if (sampleNum != getSampleOffset()) {
            setSampleOffset(sampleNum, true, true);
        }

        // Boundary-Check
        if (getSampleOffset() >= tracklen * sampleRate) {
            long maxOffset = tracklen * sampleRate - getBuffer()->getNumSamples();
            maxOffset = snapToQuarterNote(maxOffset);
            setSampleOffset(maxOffset, false, false);
        }

        // Position visuell korrigieren falls durch Snap ver�ndert
        int correctedX = (getSampleOffset() / sampleRate) * zoom;
        if (correctedX != getBounds().getX()) {
            setTopLeftPosition(correctedX, getY());
        }

        Logger::getCurrentLogger()->writeToLog("AudioRegion snapped to quarter note at sample " + String(sampleOffset));
    }

    if (wasResized) {
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
    // Region::createProperties() is already called by Region::Region() during base-class
    // construction (where virtual dispatch resolves to Region::createProperties, not here).
    // Calling it again here would overwrite nameValue/nameListener raw pointers without
    // freeing the previous objects, leaking one SimpleValueSource per AudioRegion instance.
    pitchValue = new Value();
    pitchListener = new PitchListener(*pitchValue, this);
    pitchValue->setValue(pitch);
}

void AudioRegion::resizeAudio(int length)
{
    AudioSampleBuffer* temp = new AudioSampleBuffer(2, audioBuffer->getNumSamples() + length);
    temp->copyFrom(0, 0, audioBuffer->getReadPointer(0), audioBuffer->getNumSamples());
    temp->copyFrom(1, 0, audioBuffer->getReadPointer(1), audioBuffer->getNumSamples());

    delete audioBuffer;
    audioBuffer = temp;
}

double AudioRegion::getQuarterNoteLength() {
    // Berechne die L�nge einer Viertelnote in Samples
    double bpm = Project::getInstance()->getTempo();
    double quarterNoteInSeconds = 60.0 / bpm;  // Eine Viertelnote in Sekunden
    return quarterNoteInSeconds * sampleRate;  // In Samples umrechnen
}

long AudioRegion::snapToQuarterNote(long samplePosition) {
    double quarterNoteLength = getQuarterNoteLength();

    // Auf n�chste Viertelnote runden
    long quarterNoteIndex = (long)std::round(samplePosition / quarterNoteLength);
    return (long)(quarterNoteIndex * quarterNoteLength);
}

void AudioRegion::updateSnapSettings() {
    double quarterNoteLength = getQuarterNoteLength();
    double quarterNotePixels = (quarterNoteLength / sampleRate) * zoom;

    // Raster auf Viertelnoten setzen
    if (dragger != nullptr) {
        dragger->setRaster(quarterNotePixels);
    }
    if (constrainer != nullptr) {
        constrainer->setRaster(quarterNotePixels);
    }
}


