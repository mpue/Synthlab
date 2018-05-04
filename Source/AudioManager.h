//
//  Project.hpp
//  Synthlab - App
//
//  Created by Matthias Pueski on 16.04.18.
//

#ifndef AudioManager_h
#define AudioManager_h



#include "../JuceLibraryCode/JuceHeader.h"


class AudioManager {
public:
    static AudioManager* getInstance() {
        if (instance == NULL) {
            instance = new AudioManager();
        }
        return instance;
        
    }
    
    static void destroy() {
        delete instance;
    }

    
    juce::AudioFormatManager* getFormatManager() {
        return manager;
    }
    
    juce::AudioDeviceManager* getDeviceManager() {
        return deviceManager;
    }
    
    void setDeviceManager(juce::AudioDeviceManager* manager){
        this->deviceManager = manager;
    }

    int getNumInputChannels() {
        return numInputChannels;
    }
    
    int getNumOutputChannels() {
        return numOutputChannels;
    }
    
    int getNumActiveInputs() {
        return numActiveInputs;
    }
    
    int getNumActiveOutputs() {
        return numActiveOutputs;
    }
    
    int getNumActiveChannels(int i) {
        i = i - ((i >> 1) & 0x55555555);
        i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
        return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
    }
    
    void setupChannels() {
        juce::BigInteger activeInputChannels = deviceManager->getCurrentAudioDevice()->getActiveInputChannels();
        juce::BigInteger activeOutputChannels = deviceManager->getCurrentAudioDevice()->getActiveOutputChannels();
        
        numInputChannels = deviceManager->getCurrentAudioDevice()->getInputChannelNames().size();
        numOutputChannels = deviceManager->getCurrentAudioDevice()->getOutputChannelNames().size();
        
        numActiveInputs = getNumActiveChannels(activeInputChannels.toInteger());
        numActiveOutputs = getNumActiveChannels(activeOutputChannels.toInteger());
    }
    
    juce::StringArray getInputChannelNames() {
        return deviceManager->getCurrentAudioDevice()->getInputChannelNames();
    }
    
    juce::StringArray getOutputChannelNames() {
        return deviceManager->getCurrentAudioDevice()->getOutputChannelNames();
    }
    
    
private:
    
    AudioManager() {

        manager = new juce::AudioFormatManager();
        manager->registerBasicFormats();
   }
    
    ~AudioManager() {

        delete manager;

    }
    
    
    
    
    static AudioManager* instance;
    juce::AudioDeviceManager* deviceManager;
    
    int numInputChannels = 0;
    int numOutputChannels = 0;
    
    int numActiveInputs = 0;
    int numActiveOutputs = 0;
    
    juce::AudioFormatManager* manager;
};

#endif /* Project_hp*/
