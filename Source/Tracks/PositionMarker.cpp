/*
  ==============================================================================

    PositionMarker.cpp
    Created: 24 Nov 2016 11:00:23am
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "PositionMarker.h"
#include "MainComponent.h"

//==============================================================================
PositionMarker::PositionMarker(double lengthInSeconds)
{
    this->length = lengthInSeconds;
    // startTimer(100);
	this->audioPosition = 0;
	this->drawPosition = 0;
    setInterceptsMouseClicks(false, false);
}

PositionMarker::~PositionMarker()
{
}

void PositionMarker::paint (Graphics& g)
{
	if (length > 0) {
		this->drawPosition = (audioPosition / length) * this->width + this->x;
	}
    setTopLeftPosition(this->drawPosition, 0);
    g.setColour(Colours::steelblue.darker());
    g.fillAll();
}

void PositionMarker::setSize(int width, int height) {
    Component::setSize(2, height);
}

void PositionMarker::resized()
{
}

void PositionMarker::setLength(double lengthInSeconds) {
    this->length = lengthInSeconds;
    repaint();
}

void PositionMarker::setPosition(double position) {
	if (position < 0) {
		position = 0;
	}

    this->audioPosition = position;
    repaint();
}

double PositionMarker::getDrawPosition() {
    this->drawPosition = (audioPosition / length) * this->width + this->x;
    Logger::getCurrentLogger()->writeToLog("Marker : "+String(drawPosition));
    
    return drawPosition;
}

double PositionMarker::getPosition() {
    return audioPosition;
}

void PositionMarker::setDrawingBounds(int x, int y, int width, int height) {
    this->x = x;
    this->y = y;
    this->width = width;
    this->height = height;
}

