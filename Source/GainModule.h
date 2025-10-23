/*
  ==============================================================================

    GainModule.h
    Created: 22 Oct 2025 10:51:57am
    Author:  mpue

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"

//==============================================================================
/*
 */
class GainModule : public Module
{
public:
    GainModule(float sampleRate, int buffersize);
    ~GainModule();
    
    void paint(juce::Graphics& g) override;
    virtual void process() override;
    virtual void configurePins() override;
    
    virtual juce::String getCategory() override {
        return "Audio";
    }
    
    virtual juce::String getDescription() override {
        return "Adjusts the gain/volume of an audio signal";
    }
    
private:
    Pin* audioIn = nullptr;
    Pin* gainIn = nullptr;
    Pin* audioOut = nullptr;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GainModule)
};
