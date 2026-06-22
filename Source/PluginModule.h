#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"
#include "Plugins/PluginManager.h"
#include "AudioEngine/OfflinePlayHead.h"

class Track;  // forward decl — plugin only stores a pointer for identity routing

class PluginModule : public Module
{
public:
    PluginModule(double sampleRate, int buffersize);
    ~PluginModule() override;

    void paint(juce::Graphics& g) override;

    void process() override;
    void process(juce::MidiBuffer* externalBuffer) override;
    void process(MidiBuffer* externalBuffer, int numSamples) override;
    void processInternal(juce::MidiBuffer* externalBuffer, int numSampkes) override;
    void configurePins() override;

    // Audio-Device-Aenderung (z.B. Buffer-Size aus AudioSettings) durchreichen,
    // sonst arbeitet das geladene Plugin weiter mit der zur Load-Zeit
    // initialisierten Block-Groesse und produziert Glitches / Latenz-Versatz.
    void setBuffersize(int buffersize) override;
    void setSampleRate(float rate) override;
    juce::String getCategory() override { return "FX"; }

    void setCurrentProgram(int program);
    int getCurrentProgram();

    void selectPlugin(juce::String name);
    juce::String getPluginName();
	AudioPluginInstance* getPlugin() { return plugin.get(); }
    void openPluginWindow();

    // ---- Dockable plugin editors (opt-in via Settings::dockPluginWindows) ----
    // PluginModule lives in the module/audio layer and knows nothing about the
    // UI's DockManager. MainComponent installs these hooks so a plugin editor
    // can be shown as a dock panel instead of a floating DocumentWindow. They
    // are keyed by the PluginModule pointer so each module maps to one panel.
    //   activateDockedEditorHook: focus this module's panel if it exists -> true.
    //   dockEditorHook:           create a panel that owns `editor` -> true on success.
    //   closeDockedEditorHook:    close this module's panel (called from the dtor
    //                             so the editor is destroyed before `plugin`).
    static std::function<bool(PluginModule*)> activateDockedEditorHook;
    static std::function<bool(PluginModule*, juce::AudioProcessorEditor*, const juce::String&)> dockEditorHook;
    static std::function<void(PluginModule*)> closeDockedEditorHook;
    void eventReceived(Event* e) override;
    void addMidiMessage(const juce::MidiMessage& msg);
    void resetPlugin();

    // Identity-based live routing (mirrors MidiGate): a plugin that belongs to
    // an instrument track receives live CC by Track identity rather than MIDI
    // channel, so several instruments can share a channel. A null track means
    // "modular plugin" -> classic broadcast. trackResolved distinguishes
    // "known to have no track" from "not yet looked up".
    void   setTrack(Track* t) { track = t; trackResolved = true; }
    Track* getTrack() const { return track; }
    bool   isTrackResolved() const { return trackResolved; }
    void   invalidateTrack() { track = nullptr; trackResolved = false; }

    juce::Array<juce::PropertyComponent*>& getProperties() override;
    void createProperties() override;

    juce::String getPluginState();
    void setPluginState(juce::String state);

	bool isBounceMode() const { return bounceMode; }
    void setBounceMode(bool mode) { bounceMode = mode; }

    void setCurrentSample(int64_t sample) { nextSamplePosition.store(sample); }

	OfflinePlayHead* getPlayHead() { return playHead; }

    void requestReset() { needsReset.store(true); }
    void requestPositionReset(int64_t pos) { nextSamplePosition.store(pos); }

    // Master-FX-Pfad: anders als process() hängt dieses Plugin nicht im
    // Modul-Graph, daher gibt's hier keinen eigenen Sample-Counter. Der
    // Aufrufer (MasterChannelPanel via MainComponent/BounceThread) muss die
    // Transport-Position mitliefern, sonst bekommt das Plugin keinen Playhead-
    // Update und tempo-/positions-abhängige Plugins (z.B. Output Coproducer)
    // bleiben stumm.
    void processBuffer(juce::AudioBuffer<float>& buf,
                       juce::int64 samplePosition,
                       bool isPlaying,
                       bool isRecording);
    
private:
    std::vector<std::unique_ptr<juce::PropertyComponent>> ownedProperties;
    juce::Array<juce::PropertyComponent*> propertyPtrs;


    struct PluginValueListener : juce::Value::Listener
    {
        PluginValueListener(juce::Value& v, PluginModule* m) : m(m), value(v)
        {
            value.addListener(this);
        }

        void valueChanged(juce::Value&) override
        {
            juce::String pname = value.getValue().toString();
            m->selectPlugin(pname);
        }

        PluginModule* m;
        juce::Value value;
    };
    
    std::atomic<bool> needsReset { false };
    std::atomic<int64_t> nextSamplePosition { -1 };  // -1 = no change

    std::unique_ptr<float[]> bufferLeft;
    std::unique_ptr<float[]> bufferRight;
    std::unique_ptr<juce::AudioBuffer<float>> audioBuffer;

    std::unique_ptr<juce::AudioPluginInstance> plugin;
    std::unique_ptr<PluginManager::PluginWindow> win;

    juce::Value* pluginValue;
    juce::PropertyComponent* pluginProp = nullptr;
    PluginValueListener* pluginListener;
    std::unique_ptr<juce::Label> nameLabel;

    juce::MidiBuffer midiBuffer;
    juce::String pluginName;
    juce::Array<juce::PropertyComponent*> properties;

    int currentProgram = 0;
    std::atomic<int64_t> currentSample { 0 };

    // Sidechain-Status. 0 = kein zweiter Input-Bus / Plugin akzeptiert keine
    // SC-Konfiguration; 1 = Mono-Sidechain; 2 = Stereo-Sidechain. scLeft/Right-
    // PinIdx zeigen auf die nachgereichten Pins in `pins` (Module::addPin
    // pusht sie an's Ende). -1 = Pin nicht existent (Mono-SC hat kein
    // scRightPinIdx).
    int sidechainChannels = 0;
    int scLeftPinIdx  = -1;
    int scRightPinIdx = -1;

    // Versucht den zweiten Input-Bus des frisch geladenen Plugins auf Stereo
    // (Fallback Mono) zu konfigurieren. Muss VOR prepareToPlay laufen, sonst
    // bleibt das Plugin auf seinem Default-Layout. Setzt sidechainChannels.
    void trySetupSidechainBus(juce::AudioPluginInstance& p);

    // Legt SC L (+ ggf. SC R) als IN-Pins am Modul an. Aufruf erst nachdem
    // trySetupSidechainBus sidechainChannels > 0 gesetzt hat.
    void addSidechainPins();

    double sampleRate;
    int buffersize;

    int internalLatency = 8192;

    juce::CriticalSection lock;     // guards plugin lifetime (processBlock / selectPlugin)
    juce::CriticalSection midiLock; // guards midiBuffer (eventReceived vs processInternal)

    OfflinePlayHead* playHead;

	bool bounceMode = false;

    Track* track{ nullptr };       // owning track for identity routing
    bool   trackResolved{ false }; // false until the graph has been walked

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginModule)
};
