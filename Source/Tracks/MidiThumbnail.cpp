#include "../JuceLibraryCode/JuceHeader.h"
#include "MidiThumbnail.h"
#include "../Project/Project.h"

MidiThumbnail::MidiThumbnail(double sampleRate)
    : sampleRate(sampleRate), zoom(1.0f), verticalZoom(1.0f)    
{
    midiBuffer = new MidiBuffer();
}

MidiThumbnail::~MidiThumbnail()
{
    delete midiBuffer;
}

void MidiThumbnail::paint(Graphics& g)
{
    g.fillAll(Colours::white);
    g.setColour(Colours::grey);
    g.drawRect(getLocalBounds(), 1);
    g.setColour(Colours::orange);
    g.setFont(14.0f);
    g.drawText("MidiThumbnail", getLocalBounds(), Justification::centred, true);
}

void MidiThumbnail::resized()
{
    // nothing to resize
}

double MidiThumbnail::getTotalLength()
{
    return sampleRate > 0 ? (totalSamples / sampleRate) : 0;
}

void MidiThumbnail::drawChannels(Graphics& g, double startTimeSeconds, double endTimeSeconds, float verticalZoomFactor)
{
    if (totalSamples == 0 || sampleRate <= 0.0)
        return;

    MidiMessage message;
    int sampleNum = 0;
    MidiBuffer::Iterator iterator(*midiBuffer);

    g.setColour(Colours::black);

    double timeRange = endTimeSeconds - startTimeSeconds;
    if (timeRange <= 0)
        return;

    int width = getWidth();
    int height = getHeight();
    float pixelsPerSecond = (width / timeRange);

    std::map<int, int> activeNotes;

    while (iterator.getNextEvent(message, sampleNum)) {
        double eventTime = sampleNum / sampleRate;

        if (message.isNoteOn()) {
            activeNotes[message.getNoteNumber()] = sampleNum;
        }
        else if (message.isNoteOff()) {
            int note = message.getNoteNumber();
            auto it = activeNotes.find(note);
            if (it != activeNotes.end()) {
                int startSample = it->second;
                int endSample = sampleNum;
                double startTime = startSample / sampleRate;
                double endTime = endSample / sampleRate;

                if (endTime < startTimeSeconds || startTime > endTimeSeconds) {
                    activeNotes.erase(it);
                    continue;
                }

                int x = static_cast<int>((startTime - startTimeSeconds) * pixelsPerSecond);
                int w = static_cast<int>((endTime - startTime) * pixelsPerSecond);
                w = std::max(2, w); // at least 2 pixels wide

                float normNote = static_cast<float>(note) / 127.0f;
                int y = static_cast<int>((1.0f - normNote) * height * verticalZoomFactor);

                g.fillRect(x, y, w, 4);
                activeNotes.erase(it);
            }
        }
    }
}

void MidiThumbnail::addMessage(int sampleNum, MidiMessage* message)
{
    juce::ScopedLock lock(_lock);
    midiBuffer->addEvent(*message, sampleNum);

    if (sampleNum > maxSampleNum)
        maxSampleNum = sampleNum;

    totalSamples = maxSampleNum;

    juce::MessageManager::callAsync([this]() { repaint(); });
}

void MidiThumbnail::removeAllMessages()
{
    juce::ScopedLock lock(_lock);
    midiBuffer->clear();
    maxSampleNum = 0;
    totalSamples = 0;

    juce::MessageManager::callAsync([this]() { repaint(); });
}

void MidiThumbnail::setZoom(float newZoom)
{
    zoom = newZoom;
    repaint();
}

void MidiThumbnail::setVerticalZoom(float newZoom)
{
    verticalZoom = newZoom;
    repaint();
}
