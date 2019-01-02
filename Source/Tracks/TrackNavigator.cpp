/*
  ==============================================================================

    WaveNavigator.cpp
    Created: 24 Nov 2016 8:04:54am
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "TrackNavigator.h"
#include "AudioRegion.h"
#include "MidiRegion.h"
#include "../Project/Project.h"
#include "Session.h"
#include "../Mixer.h"
#include <iterator>
#include <iostream>

using namespace std;

//==============================================================================
TrackNavigator::TrackNavigator(PositionMarker* marker, AudioDeviceManager *deviceManager)
{
    this->deviceManager = deviceManager;
    this->selector = new WaveSelector();
    this->selector->addChangeListener(this);
    this->selector->setSize(getWidth(), getHeight());
    this->selector->setBounds(0, 0, getWidth(), getHeight());
    addAndMakeVisible(selector);
    this->zoom = Project::getInstance()->getTempo() / 4;
    this->marker = marker;
	this->position = 0;
    this->dragger = new MultiComponentDragger();
    Project::getInstance()->addChangeListener(this);
    addChangeListener(Mixer::getInstance());
    
    // setInterceptsMouseClicks(true, true);
}

TrackNavigator::~TrackNavigator()
{
    selector = nullptr;
    
    for(std::vector<Track*>::iterator it = tracks.begin(); it != tracks.end(); ++it) {
        delete *it;
    }
    delete dragger;
}

void TrackNavigator::paint (Graphics& g)
{

    // g.fillAll (Colours::lightsalmon);
    /*
    for(int i = 0; i < tracks.size();i++) {		
        tracks.at(i)->paint(g);
    }
     */
}

void TrackNavigator::clearTracks() {
    for(std::vector<Track*>::iterator it = tracks.begin(); it != tracks.end(); ++it) {
        delete *it;
    }
    tracks.clear();
}

float TrackNavigator::getZoom()
{
	return this->zoom;
}

void TrackNavigator::setZoom(float zoom) {
    this->zoom = zoom;
    for(int i = 0; i < tracks.size();i++) {
        tracks.at(i)->setZoom(zoom);
    }
    // constrainer.setRaster(this->zoom / 4);
    // constrainer.setMaxX(getMaxLength() * this->zoom);
}

void TrackNavigator::updateTrackLayout(ChangeBroadcaster * source)
{
	int index = -1;

	for (int i = 0; i < tracks.size();i++) {
		if (source == tracks.at(i) && i < tracks.size() - 1) {
			index = i;
			break;
		}
	}

	if (index >= 0 && tracks.size() > index + 1) {

		for (int i = index + 1; i < tracks.size();i++) {
			tracks.at(i)->setTopLeftPosition(tracks.at(i - 1)->getX(), tracks.at(i - 1)->getPosition().getY() + tracks.at(i - 1)->getHeight());
		}
	
	}
    else {
        
        if(tracks.size() == 1) {
            tracks.at(0)->setTopLeftPosition(0,0);
        }
        
    }
    
	adjustHeight();
	sendChangeMessage();
}

void TrackNavigator::adjustHeight()
{
	int height = 0;

	for (int i = 0; i < tracks.size();i++) {
		height += tracks.at(i)->getHeight();
	}
    
    setSize(getWidth(), height);
}

double TrackNavigator::getPosition() {
    return position;
}

void TrackNavigator::setPosition(double position) {
    this->position = position;
}

double TrackNavigator::getMaxLength() {
    double maxlen = 0;
    
    for (int i = 0; i < tracks.size();i++) {

        double len = tracks.at(i)->getMaxLength();
        
        if (len > maxlen) {
            maxlen = len;
        }
    }

    return maxlen;
}

void TrackNavigator::setPlaying(bool playing) {
    this->playing = playing;
    Session::getInstance()->setRecordingStartTime();
    sendChangeMessage();
    for (int i = 0; i < tracks.size();i++) {
        for(int j = 0 ; j < tracks.at(i)->getRegions().size();j++) {
            tracks.at(i)->getRegions().at(j)->reset();
        }
    }
}

bool TrackNavigator::isPlaying() {
    return playing;
}

void TrackNavigator::setRecording(bool recording) {
    if (recording) {
        Session::getInstance()->setRecordingStartTime();
        recordStart = marker->getPosition() * Project::getInstance()->getSampleRate();
        for (int i = 0; i < tracks.size();i++) {
            if (tracks.at(i)->isRecording()) {
                
                if(tracks.at(i)->getType() == Track::Type::MIDI) {
                    tracks.at(i)->addMidiRegion(Project::getInstance()->getSampleRate(), recordStart, 0);
                }
                else if(tracks.at(i)->getType() == Track::Type::AUDIO) {
                    tracks.at(i)->addRegion(tracks.at(i)->getRecordingBuffer(),Project::getInstance()->getSampleRate(), recordStart, 0);
                }
                
                tracks.at(i)->getCurrentRecorder()->startRecording();
            }
        }
    }
    else {
        Session::getInstance()->setRecordingStopTime();
        recordStop  = marker->getPosition() * Project::getInstance()->getSampleRate();
        for (int i = 0; i < tracks.size();i++) {
            if (tracks.at(i)->isRecording()) {
                // tracks.at(i)->addRegion(tracks.at(i)->getRecordingBuffer(),Project::getInstance()->getSampleRate(), recordStart, recordStop - recordStart);
                
                
                tracks.at(i)->getCurrentRecorder()->stopRecording();
                
            }
        }
    }

    this->recording = recording;
}

bool TrackNavigator::isRecording() {
    return recording;
}

std::vector<Track*> TrackNavigator::getTracks() {
    return this->tracks;
}

Track* TrackNavigator::addTrack(TrackConfig* tc) {
    
    Track::Type type;
    
    if (tc->getType() == "audio") {
        type = Track::Type::AUDIO;
    }
    else if (tc->getType() == "midi") {
        type = Track::Type::MIDI;
    }
    
    Track* track = new Track(type, Project::getInstance()->getSampleRate(), this->dragger);
    
    track->setName(tc->getName());
    track->setMidiOutputDevice(tc->getMidiInputDevice());
    track->setMidiInputDevice(tc->getMidiInputDevice());
    track->setInputChannels(tc->getInputChannels());
    track->setOutputChannels(tc->getOutputChannels());
    track->setMute(tc->isMute());
    track->setSolo(tc->isSolo());
    track->setMono(tc->isMono());
    track->setVolume(tc->getVolume());
    track->setGain(1.0f);
    track->setPan(tc->getPan());
    track->setMidiChannel(tc->getMidiChannel(), false);
    track->setHeight(tc->getHeight());
    
    addTrack(track);
    
    int height = 1;
    
    for (int i = 0; i < this->tracks.size();i++) {
        height += tracks.at(i)->getHeight();
    }
    
    setBounds(getX(), getY(), getMaxLength() * this->zoom, height);
    
    this->marker->setSize(2, getHeight());
    this->marker->setLength(getMaxLength());
    this->selector->setSize(getWidth(), getHeight());
    
    // constrainer.setRaster(this->zoom / 4);
    // constrainer.setMaxX(getMaxLength() * this->zoom);
    
    repaint();
    sendChangeMessage();
    
    return track;
    
}

void TrackNavigator::addTrack(Track* track) {

    // deselect all tracks before adding a new one
    
    for (int i = 0; i < tracks.size();i++) {
        tracks.at(i)->setSelected(false);
    }
    
    // calculate y position of new track
    
    int yPos = 0;
    
    for (int i = 0; i < this->tracks.size();i++) {
        yPos += tracks.at(i)->getHeight();
    }
    
    // set bounds accordingly
    
    Project* p = Project::getInstance();
    track->setBounds(0, yPos, p->getTrackLength() * this->zoom, track->getHeight());
    
    this->tracks.push_back(track);
    this->currentTrack = track;
    
    addAndMakeVisible(track);
    
    track->addChangeListener(this);
    
    Mixer::getInstance()->addTrack(track);
    
    if (zoom > 0)
        track->setZoom(zoom);
    
    track->setSelected(true);

    this->selector->toFront(false);
    
    repaint();
}

Track* TrackNavigator::addTrack(Track::Type type, double sampleRate) {
    
    Track* track = new Track(type, sampleRate, this->dragger);
    
    addTrack(track);
    
	// int height = this->getParentComponent()->getHeight();
    
    int height = 1;
    
    for (int i = 0; i < this->tracks.size();i++) {
        height += tracks.at(i)->getHeight();
    }
    
    setBounds(getX(), getY(), getMaxLength() * this->zoom, height);

	this->marker->setSize(2, getHeight());
	this->marker->setLength(getMaxLength());
    this->selector->setSize(getWidth(), getHeight());
    
    // constrainer.setRaster(this->zoom / 4);
    // constrainer.setMaxX(getMaxLength() * this->zoom);
    
    repaint();
	sendChangeMessage();
    
    return track;
}

void TrackNavigator::removeSelectedTrack()
{
	for (std::vector<Track*>::iterator it = tracks.begin(); it != tracks.end();) {
		if ((*it)->isSelected()) {
			delete * it;
			it = tracks.erase(it);

			break;
		}
		else {
			++it;
		}
	}

	if (tracks.size() > 0) {

		tracks.at(0)->setTopLeftPosition(tracks.at(0)->getX(), 0);

		for (int i = 1; i < tracks.size();i++) {
			tracks.at(i)->setTopLeftPosition(tracks.at(i - 1)->getX(), tracks.at(i - 1)->getHeight() + tracks.at(i - 1)->getY());
		}

	}

	adjustHeight();
    repaint();
	sendChangeMessage();

}

Track * TrackNavigator::getCurrentTrack()
{
	return this->currentTrack;
}

void TrackNavigator::resized()
{
    int numOfTracks = tracks.size();
    
    if (numOfTracks == 0) {
        numOfTracks = 1;
    }
    
	this->marker->setSize(2, getHeight());
	this->marker->setDrawingBounds(0, 25, getWidth(), getHeight());
    repaint();
}

WaveSelector* TrackNavigator::getSelector() {
    return this->selector.get();
}

void TrackNavigator::changeListenerCallback(ChangeBroadcaster * source) 
{
    
	updateTrackLayout(source);
	
    if (source == Project::getInstance()) {
        tracklength = Project::getInstance()->getTrackLength();
        
        for (int i = 0; i < tracks.size();i++) {
            tracks.at(i)->setTrackLength(tracklength);
        }
    }
    if (Track* track = dynamic_cast<Track*>(source)){
        sendChangeMessage();
    }
    

}

bool TrackNavigator::keyPressed(const KeyPress& key, Component* originatingComponent) {

    if (key == KeyPress::spaceKey) {
        if (isPlaying())
        {
            setPlaying(false);
        }
        else
        {
            setPlaying(true);
        }
        
    }
    if (key.getTextCharacter() == '+') {
        if (getMaxLength() == 0) {
            return false;
        }
        this->zoom += Project::getInstance()->getTempo() / 8;
		setSize(getMaxLength() * this->zoom, getHeight());
        setZoom(zoom);
		repaint();
        sendChangeMessage();
    }
    else if (key.getTextCharacter() == '-') {
        if (getMaxLength() == 0) {
            return false;
        }
        this->zoom -= Project::getInstance()->getTempo() / 8;
        setSize(getMaxLength() * this->zoom, getHeight());
        setZoom(zoom);
		repaint();
        sendChangeMessage();
    }
    
    ModifierKeys m = key.getModifiers();
    
    if(m.isCommandDown()) {
        
        Logger::getCurrentLogger()->writeToLog(String(key.getKeyCode()));
        
        if (key.getKeyCode() == 76) { // L
            for (int i = 0; i < tracks.size();i++) {
                tracks.at(i)->toggleLoopSelection();
            }
        }
        else if (key.getKeyCode() == 67) { // C
            for (int i = 0; i < tracks.size();i++) {
                tracks.at(i)->copySelectedRegions();
            }
        }
        else if (key.getKeyCode() == 68) { // D
            for (int i = 0; i < tracks.size();i++) {
                tracks.at(i)->duplicateSelectedRegions();
            }
        }
        else if (key.getKeyCode() == 88) { // X
            for (int i = 0; i < tracks.size();i++) {
                tracks.at(i)->removeSelectedRegions(true);
            }
        }
        else if (key.getKeyCode() == 84) { // T
            for (int i = 0; i < tracks.size();i++) {
                tracks.at(i)->splitRegion();
            }
        }
    }
    
    return true;
}

void TrackNavigator::mouseUp (const MouseEvent& event) {
}

void TrackNavigator::mouseDrag(const MouseEvent& event) {
    if (event.mods.isLeftButtonDown() && event.mods.isAltDown()) {
        
        int start = dragger->snap(event.getMouseDownPosition().getX(), zoom / 4);
        int end = dragger->snap(event.getOffsetFromDragStart().getX(), zoom / 4);
        
        getSelector()->setSelectedRange(start,end);
    }
}

void TrackNavigator::mouseMove(const juce::MouseEvent &event ) {
    
}

void TrackNavigator::mouseDown (const MouseEvent& event) {

    int x = event.x;
    int y = event.y;
    
    
    if (!event.mods.isCtrlDown() && event.mods.isRightButtonDown()) {
        float pos = (float)dragger->snap(x,zoom/4) / getWidth();
        double total = getMaxLength();
        double relative = total * pos;
        setPosition(relative);
        marker->setPosition(position);
        for (int i = 0; i < tracks.size();i++) {
            tracks.at(i)->setCurrentMarkerPosition(marker->getDrawPosition());
        }
        sendChangeMessage();
    }
    //else
    if (Track* r = dynamic_cast<Track*>(event.eventComponent)) {
        for (int i = 0; i < tracks.size();i++) {
            tracks.at(i)->setSelected(false);
            tracks.at(i)->clearSelection();
        }

        r->setSelected(true);
        currentTrack = r;
        deviceManager->setDefaultMidiOutput(currentTrack->getMidiOutputDevice());
        
        sendChangeMessage();
        repaint();
    }
    else {
        
        if (tracks.size() > 0) {
            if (event.mods.isLeftButtonDown()) {
                for (int i = 0; i < tracks.size();i++) {
                    
                    Rectangle<int> bounds =  tracks.at(i)->getBounds();
                    bounds.setY(i * Project::DEFAULT_TRACK_HEIGHT);
                    
                    if (bounds.contains(x, y)) {
                        currentTrack = tracks.at(i);
                        Rectangle<int> selectorBounds = this->selector->getBounds();
                        selectorBounds.setY(i * Project::DEFAULT_TRACK_HEIGHT);
                        this->selector->setBounds(selectorBounds);
                        this->selector->setSelectedRange(0, 0);
                        tracks.at(i)->setSelected(true);						
                        sendChangeMessage();
                    }
                    else {
                        tracks.at(i)->setSelected(false);
                        sendChangeMessage();
                    }
                    
                }
            }
            
        }
    }
    
    
    
}
