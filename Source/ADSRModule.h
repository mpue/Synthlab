/*
  ==============================================================================

    MidiGate.h
    Created: 4 Apr 2018 3:48:23pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"


//==============================================================================
/*
*/
class ADSRModule : public Module
{
public:
    ADSRModule(double sampleRate, int buffersize);
    ~ADSRModule();

    void paint (juce::Graphics& g) override;

    void process() override;
    virtual void configurePins() override;
    
    virtual juce::String getCategory() override {
        return "Sound sources";
    }
    
    void setAttack(float attack);
    void setDecay(float decay);
    void setSustain(float sustain);
    void setRelease(float release);
    
    float getAttack();
    float getDecay();
    float getSustain();
    float getRelease();
    
    bool isMono();
    void setMono(bool value);
    
    void eventReceived(Event *e) override;
    
    virtual juce::String getDescription() override {
        return BinaryData::adsr_txt;
    }
    
    virtual juce::Array<juce::PropertyComponent*>& getProperties() override;
    virtual void createProperties() override;
    
private:
    
    struct IsMonoListener : juce::Value::Listener
    {
        IsMonoListener (juce::Value& v, ADSRModule* mod) : mod(mod),  value (v) { value.addListener (this); }
        ~IsMonoListener()  {}  // no need to remove the listener
        
        void valueChanged (juce::Value& value) override {
            mod->setMono(value.toString().getIntValue() > 0);
        }
        ADSRModule* mod;
        juce::Value value;
    };
    
    juce::Value* isMonoValue;
    juce::PropertyComponent* isMonoProp;
    IsMonoListener* isMonoListener;

    juce::Image* image;
	SynthLab::ADSR* envelope;
	SynthLab::ADSR* envelopes[128];
    int currentSample = 0;
    
    // Defaults: eine kleine, aber NICHT-null Attack/Decay/Release. Ohne das
    // schnappt die Huellkurve beim allerersten Gate-On (vor dem ersten
    // setAttack-Update via Knob-Input) mit rate=0 in einem Sample auf 1.0.
    // 5 ms / 50 ms / 100 ms decken die meisten Synth-Patches als musikalisch
    // sinnvolle Startwerte ab; sobald ein A/D/R-Knob verbunden ist, ueberschreibt
    // process() diese Werte beim naechsten Block.
    float attack = 0.005f;
    float decay = 0.05f;
    float sustain = 0.8f;
    float release = 0.1f;
    
    float value = 0;
    bool gate[128] = {false};
    int voices = 0;
    
    int lastNote = 0;
    int lastVelocity = 0;
    
    bool mono = false;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ADSRModule)
};
