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
class Track;
class MidiGate;
class PluginModule;

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

    /* The currently selected/armed track. Live input is routed to this track's
       instrument by identity, so tracks no longer need a unique MIDI channel. */
    void setActiveTrack(Track* t) { activeTrack.store(t); }

    void sendGateMessage(Module*, int channel, int velocity,int note, bool on, Track* armedTrack = nullptr);
    void sendNoteMessage(Module*, int channel, int note);
    void sendPitchBendMessage(Module*, int channel, int pitch, int midiInChannel = 1, Track* armedTrack = nullptr);
    void sendControllerMessage(Module*, int targetChannel, int controller,float value, int midiInChannel, Track* armedTrack = nullptr);
    void sendMidiStartMessage(Module*, int channel);
    void sendMidiStopMessage(Module*, int channel);
    void sendMidiClockMessage(Module*, int channel);

    void processBlock(int numSamples, TrackNavigator* navigator);
    
    void startMidiClock();
    void stopMidiClock();
    void setMidiClockBPM(double bpm);
    bool isMidiClockRunning() const;

    // Vom Audio-Thread pro Block aufgerufen (in getNextAudioBlock, innerhalb
    // des graphLocks): erzeugt die faelligen 24-PPQN-Clock-Ticks fuer die
    // aktuelle Transport-Playhead-Position. Sample-genau und driftfrei.
    void generateClockTicks(juce::int64 transportSamplePos);
    void processMidiMessage(const juce::MidiMessage& message);

private:
    /** Called on the Timer thread (default: every 5 ms).
        Takes queued messages and processes them in a non-realtime context. */
    void timerCallback() override;
    // ---------------------------------------------------------------------

    /* Hotplug: when a MIDI device is connected while the app runs, enable it
       automatically so a freshly plugged-in keyboard works without a trip to
       the audio settings. */
    void handleMidiDeviceListChange();

    /* Resolve which Track (if any) a module is wired to, by walking the
       connection graph for an adjacent TrackIn. Results are cached on the
       MidiGate / PluginModule so the walk happens at most once per module. */
    Track* findTrackForModule(Module* m);
    Track* resolveGateTrack(MidiGate* gate);
    Track* resolvePluginTrack(PluginModule* plugin);

    SynthEditor* editor{ nullptr };
    juce::AudioDeviceManager* deviceManager{ nullptr };

    // Identifiers of MIDI inputs we have already seen. Anything appearing that
    // is not in here is treated as newly connected and auto-enabled.
    juce::StringArray                  knownInputIdentifiers;
    juce::MidiDeviceListConnection     midiDeviceListConnection;

    std::mutex                           queueMutex;
    std::vector<QueuedMidiMessage>       midiQueue;   // guarded by queueMutex
    std::atomic<int>                     activeChannel{ 0 };
    std::atomic<Track*>                  activeTrack{ nullptr };

    std::atomic<bool> clockRunning{ false };
    // Transport-Gate: true nur waehrend Play/Record. Wird ueber FastBus
    // ("transport_active", publiziert von TrackNavigator::setPlaying/
    // setRecording) gesetzt. Die Clock tickt nur, wenn dies true ist.
    std::atomic<bool> transportActive{ false };

    // Clock-Timing ist an die TRANSPORT-Sample-Position gekoppelt, NICHT an
    // Wall-Clock-Zeit — nur so laeuft der Sequencer exakt zum Transport. Die
    // Anzahl gefeuerter 24-PPQN-Ticks ist eine reine Funktion der Playhead-
    // Position (kein Drift, Tempo automatisch korrekt, phasen-aligned zu
    // Sample 0). Die Ticks werden sample-genau im Audio-Callback erzeugt
    // (generateClockTicks); clockTicksFired wird nur dort (Audio-Thread)
    // angefasst, clockResyncPending wird vom Message-Thread gesetzt.
    juce::int64 clockTicksFired = 0;
    std::atomic<bool> clockResyncPending{ false };
    double clockBPM = 120.0;

	bool running{ true };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiProcessor)
};
