/*  MidiProcessor.h
 *  Thread-safe, queue-based MIDI processor for JUCE
 *  © 2025 Matthias Püski
 */
#pragma once

#include <vector>
#include <mutex>
#include "../JuceLibraryCode/JuceHeader.h"
#include "Tracks/TrackNavigator.h"       // for TrackNavigator

 // Forward declarations
class SynthEditor;
class Module;
class MidiRecorderModule;

/* Lightweight wrapper that keeps the raw message and the original timestamp.
 * You can extend this later (e.g. add source device name). */
struct QueuedMidiMessage
{
    juce::MidiMessage msg;
    double            timestampSec;

    QueuedMidiMessage(const juce::MidiMessage& m,
        double tsSeconds) : msg(m), timestampSec(tsSeconds) {
    }
};

class MidiProcessor : public juce::MidiInputCallback,
    private juce::Timer
{
public:
    MidiProcessor(SynthEditor* root,
        juce::AudioDeviceManager* deviceManager);
    ~MidiProcessor() override = default;

    /* JUCE: called on high-priority MIDI thread; we *only* enqueue here. */
    void handleIncomingMidiMessage(juce::MidiInput*,
        const juce::MidiMessage&) override;

    /* Re-scans device list while app is running. */
    void refreshMidiInputs();
    void enableAllMidiInputs();
    void disableAllMidiInputs();

    /* Exposed so GUI / navigator can change active target. */
    void setActiveChannel(int channel) { activeChannel.store(channel); }

    void sendGateMessage(Module*, int channel, int velocity,int note, bool on);
    void sendNoteMessage(Module*, int channel, int note);
    void sendPitchBendMessage(Module*, int channel, int pitch);
    void sendControllerMessage(Module*, int targetChannel, int controller,float value, int midiInChannel);
    void sendMidiStartMessage(Module*, int channel);
    void sendMidiStopMessage(Module*, int channel);
    void sendMidiClockMessage(Module*, int channel);

    void processBlock(int numSamples, TrackNavigator* navigator);
    
    void startMidiClock();
    void stopMidiClock();
    void setMidiClockBPM(double bpm);
    bool isMidiClockRunning() const;
    void processMidiMessage(const juce::MidiMessage& message);

private:
    /** Called on the Timer thread (default: every 5 ms).
        Takes queued messages and processes them in a non-realtime context. */
    void timerCallback() override;

    /* The original routing / broadcasting logic – now thread-safe. */

    /* ↓↓↓ your old helper functions (unchanged) ↓↓↓ */

    // ---------------------------------------------------------------------

    SynthEditor* editor{ nullptr };
    juce::AudioDeviceManager* deviceManager{ nullptr };

    std::mutex                           queueMutex;
    std::vector<QueuedMidiMessage>       midiQueue;   // guarded by queueMutex
    std::atomic<int>                     activeChannel{ 1 };

    std::atomic<bool> clockRunning{ false };
    double clockBPM = 120.0;
    int clockIntervalMs = 0;
    juce::int64 lastClockTimeMs = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiProcessor)
};
