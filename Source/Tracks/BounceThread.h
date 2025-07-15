#pragma once

#include <JuceHeader.h>

class MainComponent;
class Track;

class BounceThread : public juce::Thread
{
public:
    BounceThread(MainComponent* mainComponent,
        const juce::File& output,
        double durationSeconds,
        std::function<void(float)> progressCallback)
        : Thread("BounceThread"),
        main(mainComponent),
        outputFile(output),
        durationInSeconds(durationSeconds),
        onProgress(progressCallback)
    {
    }

    void run() override;

private:
    MidiBuffer buildMidiBufferForTrack(Track* track, double sampleRate);
    MidiBuffer getMidiSlice(const MidiBuffer& source, int startSample, int numSamples);
    
    MainComponent* main = nullptr;
    juce::File outputFile;
    double durationInSeconds = 0.0;
    std::function<void(float)> onProgress;
};
