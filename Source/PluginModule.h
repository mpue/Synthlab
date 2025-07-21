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

    juce::Array<juce::PropertyComponent*>& getProperties() override;
    void createProperties() override;

    juce::String getPluginState();
    void setPluginState(juce::String state);

	bool isBounceMode() const { return bounceMode; }
    void setBounceMode(bool mode) { bounceMode = mode; }

	void setCurrentSample(int sample) { currentSample = sample; }

	OfflinePlayHead& getPlayHead() { return playHead; }

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
    int currentSample = 0;

    double sampleRate;
    int buffersize;

    juce::CriticalSection lock;

    OfflinePlayHead playHead;

	bool bounceMode = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginModule)
};
