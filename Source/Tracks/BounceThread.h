/*
    BounceThread.h
    Created: 16 Jul 2025
    Author: Matthias Püski

    Description:
    This class handles offline bouncing (rendering) of audio tracks
    into a WAV file. It runs in a separate thread to avoid blocking
    the UI during export. MIDI events are collected per track and
    rendered to audio in real time into a file buffer.
*/

#pragma once
#include <JuceHeader.h>

// Forward declarations to avoid unnecessary includes
class MainComponent;
class Track;

// BounceThread is responsible for rendering the audio project (offline bouncing) to a WAV file.
// It runs on a separate thread to avoid blocking the UI during export.
class BounceThread : public juce::Thread
{
public:
    // Constructor
    // - mainComponent: reference to the main application component (e.g. to access tracks, transport, etc.)
    // - output: the output file where the bounced audio should be saved
    // - durationSeconds: the total duration of the bounce in seconds
    // - progressCallback: a lambda or function to receive bounce progress (range: 0.0 - 1.0)
    BounceThread(MainComponent* mainComponent,
        const juce::File& output,
        double durationSeconds,
        std::function<void(float)> progressCallback)
        : Thread("BounceThread"),               // Name the thread for debugging purposes
        main(mainComponent),                  // Store pointer to main component
        outputFile(output),                   // Output file path
        durationInSeconds(durationSeconds),   // Total duration to bounce
        onProgress(progressCallback)          // Progress reporting callback
    {
    }

    // This function is called when the thread starts.
    // It performs the actual bounce: collecting MIDI, generating audio, and writing it to disk.
    void run() override;

private:
    // Builds a complete MidiBuffer for a given track, simulating the full sequence
    // - track: the track to process
    // - sampleRate: the current audio sample rate
    // Returns a buffer containing all MIDI events for that track
    MidiBuffer buildMidiBufferForTrack(Track* track, double sampleRate);

    // Extracts a slice from a source MidiBuffer
    // - source: the original buffer
    // - startSample: start position in samples
    // - numSamples: length of the slice in samples
    // Returns a new MidiBuffer containing only the events within the given range
    MidiBuffer getMidiSlice(const MidiBuffer& source, int startSample, int numSamples);

    MainComponent* main = nullptr;                  // Pointer to the main application component
    juce::File outputFile;                          // Target file for bounce output
    double durationInSeconds = 0.0;                 // Duration of the bounce operation
    std::function<void(float)> onProgress;          // Callback for reporting bounce progress
};
