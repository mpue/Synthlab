/*
  ==============================================================================

    PositionMarker.cpp
    Created: 24 Nov 2016 11:00:23am
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "PositionMarker.h"
#include "../MainComponent.h"

//==============================================================================
PositionMarker::PositionMarker(double lengthInSeconds)
{
    this->length = lengthInSeconds;
    this->audioPosition = 0;
    this->drawPosition = 0;
    setInterceptsMouseClicks(false, false);
}

PositionMarker::~PositionMarker()
{
}

void PositionMarker::paint(Graphics& g)
{
    g.setColour(Colours::green);
    g.fillRect(0, 0, 2, getHeight());
}

void PositionMarker::setSize(int width, int height) {
    Component::setSize(2, height);
}

void PositionMarker::resized()
{
}

void PositionMarker::setLength(double lengthInSeconds) {
    this->length = lengthInSeconds;
    updatePosition();
    repaint();
}

void PositionMarker::setPosition(double position) {
    if (position < 0) {
        position = 0;
    }
    this->audioPosition = position;
    updatePosition();
    repaint();
}

void PositionMarker::updatePosition() {
    if (length > 0) {
        int newX = static_cast<int>((audioPosition / length) * width) + x;
        setTopLeftPosition(newX, y);
    }
}

double PositionMarker::getDrawPosition() {
    drawPosition = (audioPosition / length) * width + x;
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
    updatePosition();
    setSize(2, height);
}
