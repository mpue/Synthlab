/*
  ==============================================================================\n
    TimeLine.cpp
    Created: 25 Nov 2016 10:54:33am
    Author:  pueskma

  ==============================================================================\n*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "TimeLine.h"
#include <sstream>
#include <iostream>

  //==============================================================================
TimeLine::TimeLine(double lengthInSeconds)
{
    this->length = lengthInSeconds;
    dropShadow = std::make_unique<DropShadow>(Colour::fromFloatRGBA(0, 0, 0, 0.5), 3, Point<int>(3, 3));
    dropShadower = std::make_unique<DropShadower>(*dropShadow);
    dropShadower->setOwner(this);
}

TimeLine::~TimeLine() {}

void TimeLine::paint(Graphics& g)
{
    g.fillAll(Colours::darkgrey);
    g.setColour(Colours::black);
    g.drawRect(getLocalBounds(), 1);
    g.setColour(Colours::white);

    if (this->length > 0)
    {
        const double stepSize = getWidth() / this->length;

        for (int second = 1; second < this->length; ++second)
        {
            const float xPos = static_cast<float>(stepSize * second + offset);

            g.drawLine(xPos, 0.0f, xPos, (second % 5 == 0) ? getHeight() / 2.0f : getHeight() / 3.0f);

            if (second % 10 == 0)
            {
                g.drawText(secondsToTime(second), static_cast<int>(stepSize * (second - 1) + offset), 15, 30, 10, juce::Justification::bottomRight, false);
            }
        }
    }
}

void TimeLine::setLength(double length) {
    this->length = length;
    repaint();
}

void TimeLine::resized() {}

void TimeLine::changeListenerCallback(ChangeBroadcaster* source) {}

String TimeLine::secondsToTime(int seconds)
{
    int min = (seconds % 3600) / 60;

    std::stringstream out;

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
