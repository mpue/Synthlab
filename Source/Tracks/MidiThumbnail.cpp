/*
  ==============================================================================

    MidiThumbnail.cpp
    Created: 28 Dec 2016 7:03:57pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "MidiThumbnail.h"
#include "../Project/Project.h"

//==============================================================================
MidiThumbnail::MidiThumbnail(double sampleRate)
{
    this->sampleRate = sampleRate;
    midiBuffer = new MidiBuffer();
}

MidiThumbnail::~MidiThumbnail()
{
    delete midiBuffer;
}

void MidiThumbnail::paint(Graphics& g)
{
    g.fillAll(Colours::white);   // clear the background
    g.setColour(Colours::grey);
    g.drawRect(getLocalBounds(), 1);   // draw an outline around the component
    g.setColour(Colours::orange);
    g.setFont(14.0f);
    g.drawText("MidiThumbnail", getLocalBounds(), Justification::centred, true);
}

void MidiThumbnail::resized()
{
    // No child components to resize
}

double MidiThumbnail::getTotalLength()
{
    return sampleRate > 0 ? (totalSamples / sampleRate) : 0;
}

void MidiThumbnail::drawChannels(Graphics& g, double startTimeSeconds, double endTimeSeconds, float verticalZoomFactor)
{
    if (totalSamples == 0) return;

    int sampleNum = 0;
    MidiMessage message;
    MidiBuffer::Iterator iterator(*midiBuffer);

    g.setColour(Colours::black);

    double totalTimeSeconds = totalSamples / sampleRate;
    double timeRange = endTimeSeconds - startTimeSeconds;
    if (timeRange <= 0) return;

    int width = getBounds().getWidth();
    int height = getBounds().getHeight();

    // Berechnung des Verhältnisses zwischen Zeit und Pixeln
    float pixelsPerUnit = (width / timeRange) * zoom;

    while (iterator.getNextEvent(message, sampleNum)) {
        double eventTime = sampleNum / sampleRate;  // Zeit in Sekunden
        if (eventTime < startTimeSeconds || eventTime > endTimeSeconds) continue;

        double relativePosition = (eventTime - startTimeSeconds) / timeRange;
        int x = static_cast<int>(relativePosition * (width / pixelsPerUnit)); // Berücksichtigung von Zoom

        if (message.isNoteOn()) {
            int noteHeight = juce::jlimit(0, 127, message.getNoteNumber());
            int y = height - static_cast<int>((height / 127.0) * noteHeight);
            g.fillRect(x, y, 2, 3); // Event als kleiner Strich oder Block
        }
    }
}



void MidiThumbnail::addMessage(int sampleNum, MidiMessage* message)
{
    juce::ScopedLock lock(_lock);
    midiBuffer->addEvent(*message, sampleNum);

    if (sampleNum > maxSampleNum) {
        maxSampleNum = sampleNum;
    }

    totalSamples = maxSampleNum;

    std::function<void(void)> changeLambda = [this]() { repaint(); };
    juce::MessageManager::callAsync(changeLambda);
}

void MidiThumbnail::removeAllMessages()
{
    juce::ScopedLock lock(_lock);
    midiBuffer->clear();
    maxSampleNum = 0;
    totalSamples = 0;
    
    std::function<void(void)> changeLambda = [this]() { repaint(); };
    juce::MessageManager::callAsync(changeLambda);
}