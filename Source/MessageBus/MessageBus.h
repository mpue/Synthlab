/*
  ==============================================================================

    MessageBus.h
    Created: 16 May 2018 8:52:48pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Topic.h"
#include "BusListener.h"
#include <map>


class MessageBus {
public:

    static MessageBus* getInstance() {
        if (instance == NULL) {
            instance = new MessageBus();
        }
        return instance;
        
    }
    
    static void destroy() {
        delete instance;
    }
    
    Topic* getTopic(juce::String name);

    void updateTopic(String name, float value);
    void addListener(String name, BusListener* listener);
    
private:
    
    MessageBus() {
        
    }
    
    ~MessageBus() {
        
    }
    
    static MessageBus* instance;
    std::map<juce::String,std::vector<BusListener*>> listener;
    std::map<juce::String,Topic*> topics;
    
};
