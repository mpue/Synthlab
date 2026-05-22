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

    // Dedicated lock that protects the live module graph against
    // simultaneous mutation (load/clear/add) and audio-thread iteration.
    // Audio thread takes a ScopedTryLock in MainComponent::getNextAudioBlock
    // and produces silence for the buffer if it can't acquire — main thread
    // takes a full ScopedLock during openFile / loadModule / cleanUp etc.
    //
    // We don't reuse JUCE's AudioDeviceManager::getAudioCallbackLock() for
    // this because in our setup attempts to lock it from the main thread
    // didn't reliably block the audio callback (likely because the call
    // chain dispatches via CallbackMaxSizeEnforcer before the lock is
    // acquired, and our cleanup-then-realloc window straddles that gap).
    juce::CriticalSection& getProcessGraphLock() { return processGraphLock; }
    
    void setDeviceManager(juce::AudioDeviceManager* manager){
        this->deviceManager = manager;
        setupChannels();
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
        
        if (deviceManager->getCurrentAudioDevice() != NULL  ) {
            juce::BigInteger activeInputChannels = deviceManager->getCurrentAudioDevice()->getActiveInputChannels();
            juce::BigInteger activeOutputChannels = deviceManager->getCurrentAudioDevice()->getActiveOutputChannels();
            
            numInputChannels = deviceManager->getCurrentAudioDevice()->getInputChannelNames().size();
            numOutputChannels = deviceManager->getCurrentAudioDevice()->getOutputChannelNames().size();
            
            numActiveInputs = getNumActiveChannels(activeInputChannels.toInteger());
            numActiveOutputs = getNumActiveChannels(activeOutputChannels.toInteger());
        }
        
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
		manager->clearFormats();
        delete manager;
    }
    
    static AudioManager* instance;
    juce::AudioDeviceManager* deviceManager = nullptr;
    juce::CriticalSection processGraphLock;
    
    int numInputChannels = 0;
    int numOutputChannels = 0;
    
    int numActiveInputs = 0;
    int numActiveOutputs = 0;
    
    juce::AudioFormatManager* manager;
};

#endif /* Project_hp*/
