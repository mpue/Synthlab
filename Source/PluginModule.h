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
    
    void selectPlugin(String name);
    void openPluginWindow();
    void eventReceived(Event *e) override;
private:
    
    Image* image;
    
    int currentSample = 0;
    float* buffer;
    
    float* bufferLeft;
    float* bufferRight;
    
    AudioPluginInstance* plugin;
    PluginManager::PluginWindow* win = nullptr;
    
    String pluginName;
    AudioBuffer<float>* audioBuffer;
    MidiBuffer midiBuffer;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginModule)
};


