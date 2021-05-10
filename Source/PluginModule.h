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
#include "Plugins/PluginManager.h"

//==============================================================================
/*
 */
class PluginModule : public Module
{
public:
    PluginModule(double sampleRate, int buffersize);
    ~PluginModule();
    
    void paint (juce::Graphics& g) override;
    
    void process() override;
    virtual void configurePins() override;
    virtual juce::String getCategory() override {
        return "FX";
    }
    
    void setCurrentProgram(int program);
    int getCurrentProgram();
    
    void selectPlugin(juce::String name);
    juce::String getPluginName();
    void openPluginWindow();
    void eventReceived(Event *e) override;
    
    virtual juce::Array<juce::PropertyComponent*>& getProperties() override;
    virtual void createProperties() override;
    
    juce::String getPluginState();
    void setPluginState(juce::String state);
    
private:
    
    struct PluginValueListener : juce::Value::Listener
    {
        PluginValueListener (juce::Value& v, PluginModule* m) : m(m), value (v){ value.addListener (this); }
        ~PluginValueListener()  {}  // no need to remove the listener
        
        void valueChanged (juce::Value& value) override {
            juce::String pname = value.getValue().toString();
            m->selectPlugin(pname);
        }
        PluginModule* m;
        juce::Value value;
    };
        
    juce::Image* image;
    
    int currentProgram = 0;
    
    int currentSample = 0;
    float* buffer;
    
    float* bufferLeft;
    float* bufferRight;
    
    juce::AudioPluginInstance* plugin = nullptr;
    PluginManager::PluginWindow* win = nullptr;
    
    juce::String pluginName;
    juce::AudioBuffer<float>* audioBuffer;
    juce::MidiBuffer midiBuffer;
    
    juce::Value* pluginValue;
    juce::PropertyComponent* pluginProp = nullptr;    
    PluginValueListener* pluginListener;

    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginModule)
};


