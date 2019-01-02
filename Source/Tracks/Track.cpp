/*
  ==============================================================================

    Track.cpp
    Created: 16 Dec 2016 9:40:38am
    Author:  pueskma

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "Track.h"
#include "Session.h"
#include "MasterChannelPanel.h"
#include <iostream>

//==============================================================================
Track::Track(Type type, double sampleRate, MultiComponentDragger* dragger)
{
    manager = Session::getInstance()->getAudioFormatManager();
    
    this->type = type;
	this->sampleRate = sampleRate;
    this->maxLength = Project::getInstance()->getTrackLength();
	this->name = "empty Track";
    this->volume = 1;
    this->pan = 0;
    this->audioBuffer = new AudioSampleBuffer(2,maxLength*sampleRate);
    this->dragger = dragger;
    setHeight(Project::DEFAULT_TRACK_HEIGHT);
    
}

Track::~Track()
{
	for (std::vector<Region*>::iterator it = regions.begin(); it != regions.end(); ++it) {
		delete *it;
	}

	delete this->audioBuffer;
}

void Track::setName(juce::String name ) {
    Component::setName(name);
    this->name = name;
    sendChangeMessage();
}

void Track::clearSelection() {
    for (std::vector<Region*>::iterator it = regions.begin(); it != regions.end(); ++it) {
        (*it)->setSelected(false);
    }
}

void Track::toggleLoopSelection() {
    for (std::vector<Region*>::iterator it = regions.begin(); it != regions.end(); ++it) {
        if((*it)->isSelected())
            (*it)->setLoop(!(*it)->isLoop());
    }
}

void Track::removeSelectedRegions(bool clear) {
    
    for (std::vector<Region*>::iterator it = regions.begin(); it != regions.end();) {
        if(dragger->isSelected((*it))) {
            (*it)->removeAllChangeListeners();
            if (clear)
                audioBuffer->clear((*it)->getSampleOffset(), (*it)->getNumSamples());
            delete * it;
            it = regions.erase(it);
        }
        else {
            ++it;
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
    
    for (int i = 0;i < selected.size();i++) {
        duplicateRegion(selected.at(i));
    }
    
    selected.clear();
    
}

void Track::duplicateRegion(Region *region) {
    
    if (AudioRegion* _region = dynamic_cast<AudioRegion*>(region)) {
        
        AudioRegion* duplicate = new AudioRegion(_region,*manager, sampleRate);
        duplicate->setDragger(dragger);
        Rectangle<int>* bounds = new Rectangle<int>(0, 0, _region->getThumbnail()->getTotalLength() * 20, getHeight());
        duplicate->setBounds(region->getWidth() + region->getX(), 0, region->getWidth(), region->getHeight());
        duplicate->setThumbnailBounds(bounds);
        duplicate->setLoopCount(0);
        
        this->regions.push_back(duplicate);
        this->currentRegion = duplicate;
        
        this->currentRegion->toFront(true);
        addAndMakeVisible(duplicate);
        
        clearSelection();
        dragger->setSelected(duplicate, true);
        //duplicate->setSelected(true);
        duplicate->setSampleOffset(region->getSampleOffset() + region->getNumSamples(), false, false);
        duplicate->setOffset(region->getOffset() + region->getWidth());
        duplicate->setClipRefId(_region->getClipRefid());
        duplicate->addChangeListener(this);
        
        
        if (zoom > 0)
            duplicate->setZoom(zoom);
        
        
        audioBuffer->copyFrom(0, duplicate->getSampleOffset(), *_region->getBuffer(), 0, 0, _region->getNumSamples());
        audioBuffer->copyFrom(1, duplicate->getSampleOffset(), *_region->getBuffer(), 1, 0, _region->getNumSamples());
        
        repaint();
        
    }
    
}

void Track::addRegion(String refId, File file, double sampleRate) {

	AudioRegion* region = new AudioRegion(file, refId, *manager, sampleRate);
    region->setDragger(dragger);
	Rectangle<int>* bounds = new Rectangle<int>(0, 0, region->getWidth() * zoom, getHeight());
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
    region->setSampleOffset(sampleNum,false,false);
    region->setOffset(markerPosition);
    
    
    if (zoom > 0)
        region->setZoom(zoom);
    
    
    audioBuffer->copyFrom(0, region->getSampleOffset(), *region->getBuffer(), 0, 0, region->getBuffer()->getNumSamples());
    audioBuffer->copyFrom(1, region->getSampleOffset(), *region->getBuffer(), 1, 0, region->getBuffer()->getNumSamples());

    
	repaint();
}

void Track::addRegion(juce::String refId, juce::File file, double sampleRate, long samplePosition, long regionLength) {
    AudioRegion* region = new AudioRegion(file, refId, *manager, sampleRate);
    region->setDragger(dragger);
    Rectangle<int>* bounds = new Rectangle<int>(0, 0, region->getWidth() * zoom, getHeight());
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
    
    // region->setSampleOffset(sampleNum,false,false);
    // region->setOffset(samplePosition);
    
    region->setSampleOffset(samplePosition,false,false);
    region->setOffset(samplePosition / sampleRate);
    
    if (zoom > 0)
        region->setZoom(zoom);
    
    
    audioBuffer->copyFrom(0, region->getSampleOffset(), *region->getBuffer(), 0, 0, region->getBuffer()->getNumSamples());
    audioBuffer->copyFrom(1, region->getSampleOffset(), *region->getBuffer(), 1, 0, region->getBuffer()->getNumSamples());
    
    
    repaint();
}

void Track::addMidiRegion(double sampleRate, long samplePosition,long regionLength) {
    
    MidiRegion* region = new MidiRegion(samplePosition, regionLength, sampleRate);
    region->setDragger(dragger);
    Rectangle<int>* bounds = new Rectangle<int>(0, 0, region->getThumbnail()->getTotalLength() * 20, getHeight());
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
    
    region->setSampleOffset(samplePosition,false,false);
    region->setOffset(samplePosition / sampleRate);
    
    if (zoom > 0)
        region->setZoom(zoom);
    
    repaint();
}

void Track::addRegion(AudioSampleBuffer* source, double sampleRate, long samplePosition,long regionLength) {
    
    AudioRegion* region = new AudioRegion(source, *manager, samplePosition, regionLength, sampleRate);
    region->setDragger(dragger);
    Rectangle<int>* bounds = new Rectangle<int>(0, 0, region->getThumbnail()->getTotalLength() * 20, getHeight());
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
    
    region->setSampleOffset(samplePosition,false,false);
    region->setOffset(samplePosition / sampleRate);
    
    if (zoom > 0)
        region->setZoom(zoom);
    
    audioBuffer->copyFrom(0, region->getSampleOffset(), *region->getBuffer(), 0, 0, region->getBuffer()->getNumSamples());
    audioBuffer->copyFrom(1, region->getSampleOffset(), *region->getBuffer(), 1, 0, region->getBuffer()->getNumSamples());
    
    
    repaint();
}

void Track::splitRegion() {

    long sampleNum = (this->maxLength / (this->maxLength * zoom)) * markerPosition * sampleRate;
    
    if (AudioRegion* region = dynamic_cast<AudioRegion*>(getCurrentRegion(sampleNum))) {
        
        if (region ==  NULL) {
            return;
        }
        
        if (region->isSelected()) {
            
            long numLeftSamples = sampleNum - region->getSampleOffset();
            long numRightSamples = region->getNumSamples() - numLeftSamples;
            
            AudioRegion* leftRegion = new AudioRegion(region,*manager,sampleRate,0,numLeftSamples);
            AudioRegion* rightRegion = new AudioRegion(region,*manager,sampleRate,numLeftSamples, numRightSamples);
            
            leftRegion->setDragger(dragger);
            rightRegion->setDragger(dragger);
            
            Rectangle<int>* leftBounds = new Rectangle<int>(0, 0, leftRegion->getThumbnail()->getTotalLength() * 20, getHeight());
            Rectangle<int>* rightBounds = new Rectangle<int>(0, 0, rightRegion->getThumbnail()->getTotalLength() * 20, getHeight());
            
            leftRegion->setBounds(region->getX(), 0, leftRegion->getWidth(), getHeight());
            leftRegion->setThumbnailBounds(leftBounds);
            
            rightRegion->setBounds(markerPosition, 0, rightRegion->getWidth(), getHeight());
            rightRegion->setThumbnailBounds(rightBounds);
            
            this->regions.push_back(leftRegion);
            leftRegion->addChangeListener(this);
            this->regions.push_back(rightRegion);
            rightRegion->addChangeListener(this);
            
            addAndMakeVisible(leftRegion);
            addAndMakeVisible(rightRegion);
            
            leftRegion->setSampleOffset(region->getSampleOffset(),false,false);
            leftRegion->setOffset(region->getOffset());
            
            rightRegion->setSampleOffset(sampleNum,false,false);
            rightRegion->setOffset(markerPosition);
            
            if (zoom > 0) {
                leftRegion->setZoom(zoom);
                rightRegion->setZoom(zoom);
            }
            
            clearSelection();
            
            region->setSelected(true);
            dragger->setSelected(region, true);
            
            removeSelectedRegions(false);
            
            leftRegion->setSelected(true);
            rightRegion->setSelected(true);
        }
        
    }
    
}

void Track::setZoom(float zoom)
{
    this->zoom = zoom;
    
    Logger::getCurrentLogger()->writeToLog(String(zoom));
    
    for (std::vector<Region*>::iterator it = regions.begin(); it != regions.end(); ++it) {
        (*it)->setZoom(zoom);
    }
    
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
    // sendChangeMessage();
}

float Track::getVolume()
{
	return volume;
}

void Track::setPan(float pan)
{
    this->pan = pan;
    // sendChangeMessage();
}

float Track::getPan()
{
    return pan;
}

Region* Track::getCurrentRegion(long sample) {
    
    Region* current = NULL;
    
    for (std::vector<Region*>::iterator it = regions.begin(); it != regions.end(); ++it) {
        
        Region* r = *it;
        
        long len = r->getNumSamples();
        long offset = r->getSampleOffset();
        
        if (sample >= offset && sample < (offset + len)) {
            current = r;
            break;
        }
        
    }
    
    /*
    if (current == NULL) {
        Logger::getCurrentLogger()->writeToLog("No region found.");
    }
     */
    
    return current;
    
}

const float Track::getSample(int channel, long sample) {
    if (mute) {
        return 0;
    }
    return audioBuffer->getSample(channel,sample);
}

void Track::addMessage(MidiMessage* message,double time, int sampleNum) {

    Region* r;
    
    if (regions.size() == 1) {
        r = regions.at(0);
    }
    else {
        r = getCurrentRegion(sampleNum);
    }

    MidiRegion* midiRegion = static_cast<MidiRegion*>(r);
    midiRegion->addMessage(message,time, sampleNum);
    this->numSamples = sampleNum;
}

MidiMessage* Track::getMessage(double time,int sampleNum) {
    
    Region* r = getCurrentRegion(sampleNum);
    
    if(r == NULL) {
        return NULL;
    }
    
    MidiRegion* midiRegion = static_cast<MidiRegion*>(r);
    return midiRegion->getMessage(time, sampleNum);
    
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
    if (channel == 0) {
        return this->magnitudeLeft;
    }
    else {
        return this->magnitudeRight;
    }
}

void Track::updateMagnitude(int sample, int buffersize,  float gainLeft, float gainRight) {
    this->magnitudeLeft = audioBuffer->getMagnitude(0, sample, buffersize) * gainLeft;
    this->magnitudeRight = audioBuffer->getMagnitude(1, sample, buffersize) * gainRight;
}

const float * Track::getReadBuffer(int channel) {

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

void Track::paint (Graphics& g) {
    
	if (selected) {
		g.setColour(Colours::lightgrey.brighter());
	}
	else {
		g.setColour(Colours::lightgrey);
	}
	
    g.fillAll();
    g.setColour(Colours::grey);
    g.drawLine(0,getHeight(),getWidth(),getHeight());
    
}
               

void Track::resized() {
    for (std::vector<Region*>::iterator it = regions.begin(); it != regions.end(); ++it) {
        (*it)->setSize((*it)->getWidth(), getHeight());
        (*it)->setZoom(zoom);
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

/*
int Track::getHeight()
{
    return this->height;
}
*/
AudioSampleBuffer * Track::getBuffer()
{
    if (type == AUDIO) {
        AudioRegion* region = static_cast<AudioRegion*>(currentRegion);
        return region->getBuffer();
    }
    
    return NULL;
}

AudioSampleBuffer * Track::getRecordingBuffer()
{
    return this->audioBuffer;
}

void Track::setCurrentMarkerPosition(int position) {
    this->markerPosition = position;
    Logger::getCurrentLogger()->writeToLog("Track " + String(midiChannel) + " marker : "+ String(position));
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

void Track::changeListenerCallback(ChangeBroadcaster * source) {
    
    if(AudioRegion* r = dynamic_cast<AudioRegion*>(source)){
        // cleanup old range first
        audioBuffer->clear(r->getOldOffset(), r->getBuffer()->getNumSamples());
        // copy data from region to tracks audio buffer
        long numSamples = r->getBuffer()->getNumSamples();
        audioBuffer->copyFrom(0, r->getSampleOffset(), *r->getBuffer(), 0, 0, r->getNumSamples());
        audioBuffer->copyFrom(1, r->getSampleOffset(), *r->getBuffer(), 1, 0, r->getNumSamples());
    }
    if(MasterChannelPanel* panel = dynamic_cast<MasterChannelPanel*>(source)){
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
    // sendChangeMessage();
}

bool Track::isRecording() {
    return recording;
}

void Track::setSolo(bool solo) {
    this->solo = solo;
    // sendChangeMessage();
}

bool Track::isSolo() {
    return solo;
}

void Track::setMute(bool mute) {
    this->mute = mute;
    // sendChangeMessage();
}

bool Track::isMute() {
    return mute;
}

void Track::setMono(bool mono) {
    this->mono = mono;
    // sendChangeMessage();
}

bool Track::isMono() {
    return mono;
}
