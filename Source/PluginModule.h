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
    
    void paint (Graphics& g) override;
    
    void process() override;
    virtual void configurePins() override;
    virtual String getCategory() override {
        return "FX";
    }
    
    void setCurrentProgram(int program);
    int getCurrentProgram();
    
    void selectPlugin(String name);
    String getPluginName();
    void openPluginWindow();
    void eventReceived(Event *e) override;
    
    virtual juce::Array<PropertyComponent*>& getProperties() override;
    virtual void createProperties() override;
    
    String getPluginState();
    void setPluginState(String state);
    
private:
    
    struct PluginValueListener : Value::Listener
    {
        PluginValueListener (Value& v, PluginModule* m) : m(m), value (v){ value.addListener (this); }
        ~PluginValueListener()  {}  // no need to remove the listener
        
        void valueChanged (Value& value) override {
            m->selectPlugin(value.getValue().toString());
        }
        PluginModule* m;
        Value value;
    };
        
    Image* image;
    
    int currentProgram = 0;
    
    int currentSample = 0;
    float* buffer;
    
    float* bufferLeft;
    float* bufferRight;
    
    AudioPluginInstance* plugin;
    PluginManager::PluginWindow* win = nullptr;
    
    String pluginName;
    AudioBuffer<float>* audioBuffer;
    MidiBuffer midiBuffer;
    
    Value* pluginValue;
    PropertyComponent* pluginProp;
    PluginValueListener* pluginListener;

    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginModule)
};


