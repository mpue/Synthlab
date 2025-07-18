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

    if (length > 0 && bpm > 0.0 && timeSigDenominator > 0)
    {
        const double secondsPerBeat = 60.0 / bpm;
        const double beatsPerBar = static_cast<double>(timeSigNumerator);
        const double secondsPerBar = secondsPerBeat * beatsPerBar;
        const int totalBars = static_cast<int>(length / secondsPerBar) + 1;

        const double pixelsPerSecond = getWidth() / length;

        for (int bar = 1; bar <= totalBars; ++bar)
        {
            const double barStartSec = (bar - 1) * secondsPerBar;

            for (int beat = 0; beat < timeSigNumerator; ++beat)
            {
                const double beatTimeSec = barStartSec + beat * secondsPerBeat;
                const float x = static_cast<float>(beatTimeSec * pixelsPerSecond + offset);

                // Längere Linie am Taktanfang
                const bool isBarStart = (beat == 0);
                float lineHeight = isBarStart ? getHeight() / 2.0f : getHeight() / 3.0f;
                g.drawLine(x, 0.0f, x, lineHeight);

                // Nur Bar-Zahlen mittig zeichnen
                if (isBarStart)
                {
                    String label = String(bar);
                    const int textWidth = 30;
                    const int textHeight = 12;
                    g.drawText(label,
                        static_cast<int>(x - textWidth / 2),
                        getHeight() - textHeight - 2,
                        textWidth, textHeight,
                        juce::Justification::centred);
                }
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

void TimeLine::setTempoAndTimeSignature(double bpm_, int numerator, int denominator)
{
    bpm = bpm_;
    timeSigNumerator = numerator;
    timeSigDenominator = denominator;
    repaint();
}


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
