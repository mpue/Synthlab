/*
  ==============================================================================

    TimeLine.cpp
    Created: 25 Nov 2016 10:54:33am
    Author:  pueskma

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "TimeLine.h"
#include <sstream>
#include <iostream>

//==============================================================================
TimeLine::TimeLine(double lengthInSeconds)
{
    this->length = lengthInSeconds;
    dropShadow = new DropShadow(Colour::fromFloatRGBA(0,0,0,0.5),3,Point<int>(3,3));
    dropShadower = new DropShadower(*dropShadow);
    dropShadower->setOwner(this);
}

TimeLine::~TimeLine()
{
    dropShadower = nullptr;
    dropShadow = nullptr;
}

void TimeLine::paint (Graphics& g)
{
    g.fillAll (Colours::darkgrey);   // clear the background
	g.setColour(Colours::black);
	g.drawRect(getLocalBounds(), 1);   // draw an outline around the component
	g.setColour(Colours::white);
	if (this->length > 0) {

		double stepSize = getWidth() / this->length;

		for (int second = 1; second < this->length;second++) {
			
			if (second % 5 == 0) {
				g.drawLine(stepSize * second + offset, 0, stepSize * second + offset, getHeight() / 2);
			}
			else {
				g.drawLine(stepSize * second + offset, 0, stepSize * second + offset, getHeight() / 3);
			}

			if (second % 10 == 0 && second > 0) {
				g.drawText(secondsToTime(second), stepSize * (second - 1) + offset, 15,30,10, juce::Justification::bottomRight, false);
			}
		}

	}

}

void TimeLine::setLength(double length) {
    this->length = length;
    repaint();
}

void TimeLine::resized()
{
    // This method is where you should set the bounds of any child
    // components that your component contains..

}


void TimeLine::changeListenerCallback(ChangeBroadcaster * source)
{


}

String TimeLine::secondsToTime(int seconds)
{
	int min = (seconds % 3600) / 60;

	std::stringstream out;

	/*
	if (min < 10) {
		out << "0";
	}
	*/

	out << min << ":";

	int sec = (seconds % 3600) % 60;

	if (sec < 10) {
		out << "0";
	}

	out << sec;

	return String(out.str());
}

void TimeLine::setOffset(int offset)
{
	this->offset = offset;
	repaint();
}
