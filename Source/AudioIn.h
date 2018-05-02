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
class AudioIn    : public Module
{
public:
    AudioIn();
    ~AudioIn();
    
    void paint (Graphics& g) override;
    void setDeviceManager(AudioDeviceManager* manager);
    virtual void process() override;
    
    virtual void configurePins() override;
    
    virtual String getCategory() override {
        return "Input / Output";
    }
    
    // this is needed to associate the channel with the mixer
    int getChannelIndex();
    void setChannelIndex(int index);
    
    
private:
    AudioDeviceManager* deviceManager;
    int channelIndex;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioIn)
};

