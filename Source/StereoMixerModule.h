/*
 ==============================================================================

 StereoMixerModule.h

 Stereo-Variante des MixerModule. Pro Kanal drei Pins:
   InL, InR (AUDIO IN), A (VALUE IN, Level 0..1).
 Summe wird auf OutL / OutR geschrieben. Anzahl Kanaele wie beim
 MixerModule ueber die "Inputs"-Property dynamisch bis 16.

 ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"

class StereoMixerModule : public Module
{
public:
    StereoMixerModule(float sampleRate, int buffersize);
    ~StereoMixerModule();

    void paint(juce::Graphics& g) override;
    virtual void process() override;
    virtual void configurePins() override;

    void setInputs(int numInputs);

    virtual juce::String getCategory() override {
        return "Audio";
    }
    virtual juce::String getDescription() override {
        return "Stereo-Mixer. Pro Kanal: InL, InR (Audio), A (Level 0..1). "
               "Outputs OutL, OutR. Anzahl Kanaele 1..16 ueber Properties.";
    }

    virtual juce::Array<juce::PropertyComponent*>& getProperties() override;
    virtual void createProperties() override;

private:
    struct InputsListener : juce::Value::Listener
    {
        InputsListener(juce::Value& v, StereoMixerModule* m) : m(m), value(v) { value.addListener(this); }
        ~InputsListener() {}
        void valueChanged(juce::Value& value) override {
            m->setInputs(value.toString().getIntValue());
        }
        StereoMixerModule* m;
        juce::Value value;
    };

    void appendChannel(int idx);

    juce::Value* inputsValue = nullptr;
    juce::PropertyComponent* inputsProp = nullptr;
    InputsListener* inputsListener = nullptr;

    int numInputs = 3;
    Pin* outL = nullptr;
    Pin* outR = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StereoMixerModule)
};
