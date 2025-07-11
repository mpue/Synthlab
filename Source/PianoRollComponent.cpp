// PianoRollComponent.cpp
#include "PianoRollComponent.h"

PianoRollComponent::PianoRollComponent(juce::MidiMessageSequence& seq)
    : midiSequence(seq)
{
    setWantsKeyboardFocus(true);
    addKeyListener(this);
}

void PianoRollComponent::paint(juce::Graphics& g)
{
    drawGrid(g);
    drawPiano(g);
    drawNotes(g);
}

void PianoRollComponent::resized() {}

void PianoRollComponent::setGridResolution(int denominator)
{
    gridResolution = 1.0f / denominator;
    repaint();
}

void PianoRollComponent::drawGrid(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);
    auto bounds = getLocalBounds();

    for (int i = 0; i < bounds.getWidth(); ++i)
    {
        double beat = (i - pianoWidth) / xZoom;
        if (beat >= 0.0 && fmod(beat, gridResolution) < 0.01)
        {
            g.setColour(juce::Colours::white.withAlpha(0.1f));
            g.drawLine((float)i, 0, (float)i, (float)bounds.getHeight());
        }
    }

    for (int p = 0; p < 128; ++p)
    {
        int y = getYForPitch(p);
        g.setColour(juce::Colours::black.withAlpha(0.2f));
        g.drawLine((float)pianoWidth, (float)y, (float)bounds.getWidth(), (float)y);
    }
}

void PianoRollComponent::drawPiano(juce::Graphics& g)
{
    for (int i = 0; i < 128; ++i)
    {
        int y = getYForPitch(i);
        g.setColour(isBlackKey(i) ? juce::Colours::black : juce::Colours::white);
        g.fillRect(0, y, pianoWidth, (int)yZoom);
        g.setColour(juce::Colours::grey);
        g.drawRect(0, y, pianoWidth, (int)yZoom);
    }
}

void PianoRollComponent::drawNotes(juce::Graphics& g)
{
    for (int i = 0; i < midiSequence.getNumEvents(); ++i)
    {
        auto* e = midiSequence.getEventPointer(i);
        if (!e || !e->message.isNoteOn()) continue;

        int offIndex = midiSequence.getIndexOfMatchingKeyUp(i);
        if (offIndex < 0 || offIndex >= midiSequence.getNumEvents()) continue;

        double start = e->message.getTimeStamp();
        double duration = midiSequence.getEventPointer(offIndex)->message.getTimeStamp() - start;
        if (duration <= 0.0) continue;

        drawNote(g, e->message, duration, selectedIndices.count(i) > 0);
    }
}

void PianoRollComponent::drawNote(juce::Graphics& g, const juce::MidiMessage& msg, double duration, bool selected)
{
    auto rect = getNoteRect(msg, duration);
    g.setColour(selected ? juce::Colours::orange : juce::Colours::lightblue);
    g.fillRect(rect);
    g.setColour(juce::Colours::black);
    g.drawRect(rect);
}

bool PianoRollComponent::isBlackKey(int n)
{
    int note = n % 12;
    return note == 1 || note == 3 || note == 6 || note == 8 || note == 10;
}

int PianoRollComponent::getYForPitch(int pitch)
{
    return (int)((127 - pitch) * yZoom);
}

double PianoRollComponent::getTimeAtX(int x)
{
    return (x - pianoWidth) / xZoom;
}

int PianoRollComponent::getPitchAtY(int y)
{
    return 127 - (int)(y / yZoom);
}

int PianoRollComponent::getXForTime(double time)
{
    return pianoWidth + (int)(time * xZoom);
}

juce::Rectangle<int> PianoRollComponent::getNoteRect(const juce::MidiMessage& msg, double duration)
{
    int x = getXForTime(msg.getTimeStamp());
    int y = getYForPitch(msg.getNoteNumber());
    int w = juce::jmax(1, (int)(duration * xZoom));
    int h = juce::jmax(1, (int)yZoom);
    return { x, y, w, h };
}

void PianoRollComponent::mouseDown(const juce::MouseEvent& e)
{
    if (e.getNumberOfClicks() == 2 && e.getPosition().getX() > pianoWidth)
    {
        int pitch = getPitchAtY(e.y);
        double start = getTimeAtX(e.x);
        double duration = 0.25; // 1/4 Note

        juce::MidiMessage on = juce::MidiMessage::noteOn(1, pitch, (juce::uint8)100);
        juce::MidiMessage off = juce::MidiMessage::noteOff(1, pitch);
        on.setTimeStamp(start);
        off.setTimeStamp(start + duration);

        midiSequence.addEvent(on);
        midiSequence.addEvent(off);

		midiSequence.updateMatchedPairs();

        repaint();
        return; // Kein Drag/Resize nach Doppelklick
    }

    dragStart = e.getPosition();
    isDragging = false;
    selectedIndices.clear();
    resizingIndex = -1;

    for (int i = 0; i < midiSequence.getNumEvents(); ++i)
    {
        auto* ev = midiSequence.getEventPointer(i);
        if (!ev || !ev->message.isNoteOn()) continue;

        int offIndex = midiSequence.getIndexOfMatchingKeyUp(i);
        if (offIndex < 0 || offIndex >= midiSequence.getNumEvents()) continue;

        double duration = midiSequence.getEventPointer(offIndex)->message.getTimeStamp() - ev->message.getTimeStamp();
        auto rect = getNoteRect(ev->message, duration);

        if (rect.contains(dragStart))
        {
            selectedIndices.insert(i);
            if (std::abs(dragStart.x - rect.getRight()) < 6)
            {
                isResizing = true;
                resizingIndex = i;
                originalDuration = duration;
            }
            break;
        }
    }

    repaint();
}

void PianoRollComponent::mouseDrag(const juce::MouseEvent& e)
{
    isDragging = true;

    auto delta = e.getPosition() - dragStart;
    double deltaTime = delta.x / xZoom;
    double deltaPitchF = -(delta.y / yZoom);

    // Runden und speichern nur bei ganzzahligen Tonhöhenänderungen
    if (isResizing && resizingIndex >= 0)
    {
        int offIndex = midiSequence.getIndexOfMatchingKeyUp(resizingIndex);
        if (offIndex < 0 || offIndex >= midiSequence.getNumEvents()) return;

        double newDuration = originalDuration + deltaTime;
        double newOffTime = midiSequence.getEventTime(resizingIndex) + std::max(0.05, newDuration);
        midiSequence.getEventPointer(offIndex)->message.setTimeStamp(newOffTime);
    }
    else
    {
        int deltaPitch = static_cast<int>(std::round(deltaPitchF));

        if (deltaTime != 0.0 || deltaPitch != 0)
        {
            for (int i : selectedIndices)
            {
                auto* noteOnEvent = midiSequence.getEventPointer(i);
                if (!noteOnEvent) continue;

                int offIndex = midiSequence.getIndexOfMatchingKeyUp(i);
                if (offIndex < 0 || offIndex >= midiSequence.getNumEvents()) continue;

                auto* noteOffEvent = midiSequence.getEventPointer(offIndex);
                if (!noteOffEvent) continue;

                double newStart = noteOnEvent->message.getTimeStamp() + deltaTime;
                double newEnd = noteOffEvent->message.getTimeStamp() + deltaTime;

                int newPitch = juce::jlimit(0, 127, noteOnEvent->message.getNoteNumber() + deltaPitch);

                noteOnEvent->message.setTimeStamp(newStart);
                noteOffEvent->message.setTimeStamp(newEnd);

                noteOnEvent->message.setNoteNumber(newPitch);
                noteOffEvent->message.setNoteNumber(newPitch);
            }

            dragStart = e.getPosition();
        }
    }

    repaint();
}


void PianoRollComponent::mouseUp(const juce::MouseEvent& e)
{

    isResizing = false;
    repaint();
}

void PianoRollComponent::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (e.mods.isShiftDown())
        xZoom *= (1.0f + wheel.deltaY);
    else
        yZoom *= (1.0f + wheel.deltaY);

    xZoom = juce::jlimit(20.0f, 800.0f, xZoom);
    yZoom = juce::jlimit(6.0f, 48.0f, yZoom);
    repaint();
}

bool PianoRollComponent::keyPressed(const juce::KeyPress& key, juce::Component*)
{
    if (key.getKeyCode() == juce::KeyPress::deleteKey)
    {
        std::vector<int> toRemove;
        for (int i : selectedIndices)
        {
            int offIndex = midiSequence.getIndexOfMatchingKeyUp(i);
            if (offIndex >= 0 && offIndex < midiSequence.getNumEvents())
            {
                toRemove.push_back(offIndex);
                toRemove.push_back(i);
            }
        }

        std::sort(toRemove.begin(), toRemove.end(), std::greater<int>());
        for (int i : toRemove)
            midiSequence.deleteEvent(i, false);

        selectedIndices.clear();
        repaint();
        return true;
    }
    return false;
}
