/*
  ==============================================================================
    TimeLine.cpp
    Created: 25 Nov 2016 10:54:33am
    Author:  pueskma
    Extended: Loop functionality between two locators with 1/4 beat snap
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

        // Haupt-Taktlinien und Beat-Linien zeichnen
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

                // 1/4-Takt Unterteilungen zeichnen (schwächere Linien)
                g.setColour(Colours::lightgrey);
                for (int quarter = 1; quarter < 4; ++quarter)
                {
                    const double quarterTimeSec = beatTimeSec + (quarter * secondsPerBeat / 4.0);
                    const float quarterX = static_cast<float>(quarterTimeSec * pixelsPerSecond + offset);
                    if (quarterX < getWidth())
                    {
                        g.drawLine(quarterX, 0.0f, quarterX, getHeight() / 6.0f);
                    }
                }
                g.setColour(Colours::white);

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

        // Loop-Region zeichnen
        if (loopEnabled && loopEnd > loopStart)
        {
            float startX = timeToPixel(loopStart);
            float endX = timeToPixel(loopEnd);

            // Loop-Region mit transparenter Farbe hervorheben
            g.setColour(Colour::fromFloatRGBA(0.0f, 1.0f, 0.0f, 0.2f));
            g.fillRect(startX, 0.0f, endX - startX, static_cast<float>(getHeight()));

            // Loop-Locatoren zeichnen
            g.setColour(Colours::green);
            const float locatorWidth = 2.0f;

            // Start-Locator (Dreieck nach rechts)
            Path startTriangle;
            startTriangle.addTriangle(startX, 0.0f,
                startX + 8.0f, 4.0f,
                startX, 8.0f);
            g.fillPath(startTriangle);
            g.drawLine(startX, 0.0f, startX, static_cast<float>(getHeight()), locatorWidth);

            // End-Locator (Dreieck nach links)  
            Path endTriangle;
            endTriangle.addTriangle(endX, 0.0f,
                endX - 8.0f, 4.0f,
                endX, 8.0f);
            g.fillPath(endTriangle);
            g.drawLine(endX, 0.0f, endX, static_cast<float>(getHeight()), locatorWidth);

            // Loop-Beschriftung
            g.setColour(Colours::white);
            String loopText = "LOOP: " + secondsToTime(static_cast<int>(loopStart)) +
                " - " + secondsToTime(static_cast<int>(loopEnd));
            g.drawText(loopText,
                static_cast<int>(startX + 10), 2,
                static_cast<int>(endX - startX - 20), 12,
                juce::Justification::left);
        }
    }
}

void TimeLine::mouseMove(const MouseEvent& event)
{
    if (!loopEnabled)
    {
        setMouseCursor(MouseCursor::NormalCursor);
        return;
    }

    const float tolerance = 5.0f; // Pixel-Toleranz für Hover-Bereich

    // Prüfen ob Maus über Loop-Start oder Loop-End Locator ist
    if (isMouseOverLocator(event, loopStart, tolerance) ||
        isMouseOverLocator(event, loopEnd, tolerance))
    {
        setMouseCursor(MouseCursor::LeftRightResizeCursor);
    }
    else if (event.getPosition().x >= timeToPixel(loopStart) &&
        event.getPosition().x <= timeToPixel(loopEnd))
    {
        // Innerhalb der Loop-Region - normaler Cursor oder Bewegungs-Cursor
        setMouseCursor(MouseCursor::DraggingHandCursor);
    }
    else
    {
        setMouseCursor(MouseCursor::NormalCursor);
    }
}

void TimeLine::mouseDown(const MouseEvent& event)
{
    if (!loopEnabled)
        return;

    double clickTime = snapToQuarterBeat(pixelToTime(static_cast<float>(event.getMouseDownX())));

    // Prüfen ob auf einen Locator geklickt wurde
    if (isMouseOverLocator(event, loopStart))
    {
        currentDragMode = DragLoopStart;
        dragStartOffset = pixelToTime(static_cast<float>(event.getMouseDownX())) - loopStart;
    }
    else if (isMouseOverLocator(event, loopEnd))
    {
        currentDragMode = DragLoopEnd;
        dragStartOffset = pixelToTime(static_cast<float>(event.getMouseDownX())) - loopEnd;
    }
    else if (clickTime >= loopStart && clickTime <= loopEnd)
    {
        currentDragMode = DragLoopRegion;
        dragStartOffset = pixelToTime(static_cast<float>(event.getMouseDownX())) - loopStart;
    }
    else
    {
        // Neue Loop-Region beginnen
        currentDragMode = DragLoopEnd;
        loopStart = clickTime;
        loopEnd = clickTime;
        dragStartOffset = 0.0;
        repaint();
    }
}

void TimeLine::mouseDrag(const MouseEvent& event)
{
    if (currentDragMode == None)
        return;

    double currentTime = pixelToTime(static_cast<float>(event.getPosition().x));
    currentTime = jmax(0.0, jmin(length, currentTime)); // Auf Timeline begrenzen

    switch (currentDragMode)
    {
    case DragLoopStart:
    {
        double newStart = snapToQuarterBeat(currentTime - dragStartOffset);
        newStart = jmax(0.0, jmin(loopEnd - getQuarterBeatDuration(), newStart));
        loopStart = newStart;
        break;
    }
    case DragLoopEnd:
    {
        double newEnd = snapToQuarterBeat(currentTime - dragStartOffset);
        newEnd = jmax(loopStart + getQuarterBeatDuration(), jmin(length, newEnd));
        loopEnd = newEnd;
        break;
    }
    case DragLoopRegion:
    {
        double loopLength = loopEnd - loopStart;
        double newStart = snapToQuarterBeat(currentTime - dragStartOffset);
        newStart = jmax(0.0, jmin(length - loopLength, newStart));
        loopStart = newStart;
        loopEnd = newStart + loopLength;
        break;
    }
    default:
        break;
    }

    repaint();
}

void TimeLine::mouseUp(const MouseEvent& event)
{
    if (currentDragMode != None && onLoopChanged)
    {
        onLoopChanged(loopStart, loopEnd, loopEnabled);
    }
    currentDragMode = None;
}

void TimeLine::setLength(double length)
{
    this->length = length;
    // Loop-Region anpassen falls sie außerhalb der neuen Länge liegt
    if (loopEnd > length)
    {
        loopEnd = length;
        if (loopStart >= loopEnd)
        {
            loopStart = jmax(0.0, loopEnd - getQuarterBeatDuration());
        }
    }
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

// Loop-Funktionen
void TimeLine::setLoopEnabled(bool enabled)
{
    loopEnabled = enabled;
    repaint();
}

void TimeLine::setLoopStart(double startTime)
{
    loopStart = snapToQuarterBeat(jmax(0.0, jmin(length, startTime)));
    if (loopStart >= loopEnd)
    {
        loopEnd = snapToQuarterBeat(jmin(length, loopStart + getQuarterBeatDuration()));
    }
    repaint();
}

void TimeLine::setLoopEnd(double endTime)
{
    loopEnd = snapToQuarterBeat(jmax(0.0, jmin(length, endTime)));
    if (loopEnd <= loopStart)
    {
        loopStart = snapToQuarterBeat(jmax(0.0, loopEnd - getQuarterBeatDuration()));
    }
    repaint();
}

void TimeLine::setLoopRegion(double startTime, double endTime)
{
    if (startTime > endTime)
        std::swap(startTime, endTime);

    loopStart = snapToQuarterBeat(jmax(0.0, jmin(length, startTime)));
    loopEnd = snapToQuarterBeat(jmax(loopStart + getQuarterBeatDuration(), jmin(length, endTime)));
    repaint();

    if (onLoopChanged)
    {
        onLoopChanged(loopStart, loopEnd, loopEnabled);
    }
}

void TimeLine::clearLoop()
{
    loopStart = 0.0;
    loopEnd = 0.0;
    repaint();

    if (onLoopChanged)
    {
        onLoopChanged(loopStart, loopEnd, loopEnabled);
    }
}

// Helper-Funktionen
double TimeLine::pixelToTime(float pixel) const
{
    if (length <= 0 || getWidth() <= 0)
        return 0.0;

    return ((pixel - offset) / getWidth()) * length;
}

float TimeLine::timeToPixel(double time) const
{
    if (length <= 0)
        return 0.0f;

    return static_cast<float>((time / length) * getWidth() + offset);
}

bool TimeLine::isMouseOverLocator(const MouseEvent& event, double locatorTime, float tolerance) const
{
    float locatorX = timeToPixel(locatorTime);
    return std::abs(event.getPosition().x - locatorX) <= tolerance;
}

// Neue Snap-Funktionen
double TimeLine::snapToQuarterBeat(double time) const
{
    if (bpm <= 0.0)
        return time;

    const double quarterBeatDuration = getQuarterBeatDuration();
    if (quarterBeatDuration <= 0.0)
        return time;

    // Auf nächstes 1/4-Beat runden
    const double quarterBeats = time / quarterBeatDuration;
    const double roundedQuarterBeats = std::round(quarterBeats);

    return roundedQuarterBeats * quarterBeatDuration;
}

double TimeLine::getQuarterBeatDuration() const
{
    if (bpm <= 0.0)
        return 0.0;

    const double secondsPerBeat = 60.0 / bpm;
    return secondsPerBeat / 4.0; // Ein Viertel eines Beats
}