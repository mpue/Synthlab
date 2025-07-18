#include "PianoRollComponent.h"

PianoRollComponent::PianoRollComponent(juce::MidiMessageSequence* seq, double sampleRate)
    : midiSequence(seq), sampleRate(sampleRate)
{
    setWantsKeyboardFocus(true);
	setMouseClickGrabsKeyboardFocus(true);
    setInterceptsMouseClicks(true, true);
    addKeyListener(this);
}

PianoRollComponent::~PianoRollComponent() = default;

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

    for (int p = 0; p < 128; ++p)
    {
        // Eine Note tiefer anzeigen:
        int y = getYForPitch(p - 1);

        if (p % 12 == 0) // C-Noten (Oktaven)
        {
            g.setColour(juce::Colours::white.withAlpha(0.15f));
            g.drawLine((float)pianoWidth, (float)y, (float)bounds.getWidth(), (float)y, 2.0f);
        }
        else
        {
            g.setColour(juce::Colours::black.withAlpha(0.2f));
            g.drawLine((float)pianoWidth, (float)y, (float)bounds.getWidth(), (float)y, 1.0f);
        }
    }


    // Musikalische Rasterlinien
    double quarterNoteSamples = (60.0 / bpm) * sampleRate;
    double barLengthSamples = quarterNoteSamples * 4.0; // 4/4-Takt

    double startSample = getTimeAtX(0);
    double endSample = getTimeAtX(bounds.getWidth());

    int firstQuarter = (int)(startSample / quarterNoteSamples);
    int lastQuarter = (int)(endSample / quarterNoteSamples);

    for (int i = firstQuarter; i <= lastQuarter; ++i)
    {
        double sampleTime = i * quarterNoteSamples;
        int x = getXForTime(sampleTime);

        // Jede 4. Viertel → Taktstrich (dick)
        if (i % 4 == 0)
        {
            g.setColour(juce::Colours::white.withAlpha(0.25f));
            g.drawLine((float)x, 0.0f, (float)x, (float)bounds.getHeight(), 2.0f); // dick
        }
        else
        {
            g.setColour(juce::Colours::white.withAlpha(0.1f));
            g.drawLine((float)x, 0.0f, (float)x, (float)bounds.getHeight(), 1.0f); // dünner
        }
    }
}

void PianoRollComponent::drawPiano(juce::Graphics& g)
{
    for (int i = 0; i < 128; ++i)
    {
        int y = getYForPitch(i);
        bool isBlack = isBlackKey(i);

        g.setColour(isBlack ? juce::Colours::black : juce::Colours::white);
        g.fillRect(0, y, pianoWidth, (int)yZoom);

        g.setColour(juce::Colours::grey);
        g.drawRect(0, y, pianoWidth, (int)yZoom);

        // Oktaven beschriften
        if (i % 12 == 0)
        {
            int octave = (i / 12) - 1; // MIDI 0 = C−1
            g.setColour(isBlack ? juce::Colours::white : juce::Colours::black);
            g.setFont(10.0f);
            g.drawText("C" + juce::String(octave), 2, y, pianoWidth - 4, (int)yZoom, juce::Justification::centredLeft);
        }
    }
}


void PianoRollComponent::drawNotes(juce::Graphics& g)
{
    for (int i = 0; i < midiSequence->getNumEvents(); ++i)
    {
        auto* e = midiSequence->getEventPointer(i);
        if (!e || !e->message.isNoteOn()) continue;

        int offIndex = midiSequence->getIndexOfMatchingKeyUp(i);
        if (offIndex < 0 || offIndex >= midiSequence->getNumEvents()) continue;

        double start = e->message.getTimeStamp();
        double duration = midiSequence->getEventPointer(offIndex)->message.getTimeStamp() - start;

        if (duration <= 0.0) continue;

        drawNote(g, e->message, duration);
    }
}

void PianoRollComponent::drawNote(juce::Graphics& g, const juce::MidiMessage& msg, double duration)
{
    auto rect = getNoteRect(msg, duration);
    g.setColour(juce::Colours::lightblue);
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
    return (int)(((127 - pitch) * yZoom) + panOffset.y);
}

int PianoRollComponent::getPitchAtY(int y)
{
    return 127 - (int)((y - panOffset.y) / yZoom);
}

int PianoRollComponent::getXForTime(double sampleTime)
{
    return pianoWidth + (int)(sampleTime * xZoom + panOffset.x);
}

double PianoRollComponent::getTimeAtX(int x)
{
    return (x - pianoWidth - panOffset.x) / xZoom;
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
    juce::Component::SafePointer<PianoRollComponent> safeThis(this);
    juce::MessageManager::callAsync([safeThis]()
        {
            if (safeThis != nullptr)
                safeThis->grabKeyboardFocus();
        });
        

    if (e.mods.isMiddleButtonDown())
    {
        isPanning = true;
        panStart = e.getPosition();
        return;
    }

    if (e.getNumberOfClicks() == 2 && e.getPosition().getX() > pianoWidth)
    {
        int pitch = getPitchAtY(e.y);
        double start = getTimeAtX(e.x);        
        double duration = sampleRate * gridResolution;

        juce::MidiMessage on = juce::MidiMessage::noteOn(1, pitch, (juce::uint8)100);
        juce::MidiMessage off = juce::MidiMessage::noteOff(1, pitch);
        on.setTimeStamp(start);
        off.setTimeStamp(start + duration);

        midiSequence->addEvent(on);
        midiSequence->addEvent(off);
        midiSequence->updateMatchedPairs();

        repaint();
        return;
    }

    dragStart = e.getPosition();
    isDragging = false;
    selectedIndices.clear();
    resizingIndex = -1;

    for (int i = 0; i < midiSequence->getNumEvents(); ++i)
    {
        auto* ev = midiSequence->getEventPointer(i);
        if (!ev || !ev->message.isNoteOn()) continue;

        int offIndex = midiSequence->getIndexOfMatchingKeyUp(i);
        if (offIndex < 0 || offIndex >= midiSequence->getNumEvents()) continue;

        double duration = midiSequence->getEventPointer(offIndex)->message.getTimeStamp() - ev->message.getTimeStamp();
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
    if (isPanning)
    {
        auto delta = e.getPosition() - panStart;
        panOffset += delta.toFloat();

        // Begrenzung: nicht weiter nach rechts als X = 0
        panOffset.x = juce::jmin(panOffset.x, 0.0f);

        panStart = e.getPosition();
        repaint();
        return;
    }


    isDragging = true;
    auto delta = e.getPosition() - dragStart;
    double rawDeltaTime = delta.x / xZoom;

    double sixteenthSamples = (60.0 / bpm) / 4.0 * sampleRate;

    // Runde den Zeitunterschied auf das nächste 16tel
    double deltaTime = std::round(rawDeltaTime / sixteenthSamples) * sixteenthSamples;

    double deltaPitchF = -(delta.y / yZoom);

    if (isResizing && resizingIndex >= 0)
    {
        int offIndex = midiSequence->getIndexOfMatchingKeyUp(resizingIndex);
        if (offIndex < 0 || offIndex >= midiSequence->getNumEvents()) return;

        double newDuration = originalDuration + deltaTime;
        double newOffTime = midiSequence->getEventTime(resizingIndex) + std::max(10.0, newDuration); // min 10 samples
        midiSequence->getEventPointer(offIndex)->message.setTimeStamp(newOffTime);
    }
    else
    {
        int deltaPitch = static_cast<int>(std::round(deltaPitchF));

        if (deltaTime != 0.0 || deltaPitch != 0)
        {
            for (int i : selectedIndices)
            {
                auto* noteOnEvent = midiSequence->getEventPointer(i);
                if (!noteOnEvent) continue;

                int offIndex = midiSequence->getIndexOfMatchingKeyUp(i);
                if (offIndex < 0 || offIndex >= midiSequence->getNumEvents()) continue;

                auto* noteOffEvent = midiSequence->getEventPointer(offIndex);
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
    isPanning = false;
    isResizing = false;

    double sixteenthSamples = (60.0 / bpm) / 4.0 * sampleRate;

    for (int i : selectedIndices)
    {
        auto* noteOn = midiSequence->getEventPointer(i);
        if (!noteOn || !noteOn->message.isNoteOn()) continue;

        int offIndex = midiSequence->getIndexOfMatchingKeyUp(i);
        if (offIndex < 0 || offIndex >= midiSequence->getNumEvents()) continue;

        auto* noteOff = midiSequence->getEventPointer(offIndex);
        if (!noteOff) continue;

        double start = noteOn->message.getTimeStamp();
        double end = noteOff->message.getTimeStamp();
        double duration = end - start;

        // Snap Start zur nächsten 16tel
        double snappedStart = std::round(start / sixteenthSamples) * sixteenthSamples;
        double snappedEnd = snappedStart + duration;

        noteOn->message.setTimeStamp(snappedStart);
        noteOff->message.setTimeStamp(snappedEnd);
    }

    repaint();
}

void PianoRollComponent::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (e.mods.isShiftDown())
        xZoom *= (1.0f + wheel.deltaY);
    else
        yZoom *= (1.0f + wheel.deltaY);

    xZoom = juce::jlimit(0.0001f, 1.0f, xZoom);
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
            int offIndex = midiSequence->getIndexOfMatchingKeyUp(i);
            if (offIndex >= 0 && offIndex < midiSequence->getNumEvents())
            {
                toRemove.push_back(offIndex);
                toRemove.push_back(i);
            }
        }

        std::sort(toRemove.begin(), toRemove.end(), std::greater<int>());
        for (int i : toRemove)
            midiSequence->deleteEvent(i, false);

        selectedIndices.clear();
        repaint();
        return true;
    }
    return false;
}
