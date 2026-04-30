#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"
#include "Plugins/PluginManager.h"
#include "AudioEngine/OfflinePlayHead.h"

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
    juce::String getCategory() override { return "FX"; }

    void setCurrentProgram(int program);
    int getCurrentProgram();

    void selectPlugin(juce::String name);
    juce::String getPluginName();
	AudioPluginInstance* getPlugin() { return plugin.get(); }
    void openPluginWindow();
    void eventReceived(Event* e) override;
    void addMidiMessage(const juce::MidiMessage& msg);
    void resetPlugin();

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

    double sampleRate;
    int buffersize;

    int internalLatency = 8192;

    juce::CriticalSection lock;     // guards plugin lifetime (processBlock / selectPlugin)
    juce::CriticalSection midiLock; // guards midiBuffer (eventReceived vs processInternal)

    OfflinePlayHead* playHead;

	bool bounceMode = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginModule)
};
