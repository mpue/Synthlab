/*
  ==============================================================================

    MidiThumbnail.h
    Created: 28 Dec 2016 7:03:57pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#ifndef MIDITHUMBNAIL_H_INCLUDED
#define MIDITHUMBNAIL_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

//==============================================================================
/*
*/
class MidiThumbnail    : public Component
{
public:
    
    enum ReadState {
        NOTE_START,
        NOTE_IN,
        NOTE_END
    };
    
    MidiThumbnail(double sampleRate);
    ~MidiThumbnail();

    void paint (Graphics&) override;
    void resized() override;

    double getTotalLength();
    
    void drawChannels (Graphics& g, double startTimeSeconds, double endTimeSeconds, float verticalZoomFactor);
    
    void addMessage(int sampleNum, MidiMessage* message);
    void removeAllMessages();
    
    void setZoom(float newZoom);
    void setVerticalZoom(float newZoom);

private:
    struct ActiveNote {
        int note;
        int startSample;
    };

    MidiBuffer* midiBuffer;
    double sampleRate;
    int totalSamples = 0;    
    juce::CriticalSection _lock;

    float zoom = 1.0f;
    float verticalZoom = 1.0f;

    long maxSampleNum = 0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiThumbnail)
};


#endif  // MIDITHUMBNAIL_H_INCLUDED