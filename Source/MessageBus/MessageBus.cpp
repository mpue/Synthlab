/*
  ==============================================================================

    MessageBus.cpp
    Created: 16 May 2018 8:52:48pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "MessageBus.h"

MessageBus* MessageBus::instance = NULL;


Topic* MessageBus::getTopic(juce::String name) {
    if (topics.find( name ) != topics.end()) {
        return topics[name];
    }
    else{
        Topic* t = new Topic();
        t->setName(name);
        topics[name] = t;
        return t;
    }
}

void MessageBus::addListener(juce::String name, BusListener* listener) {
    this->listener[name].push_back(listener);
}

void MessageBus::updateTopic(juce::String name, float value) {
    Topic* t = getTopic(name);
    t->setValue(value);
    
    for (int i = 0; i < listener[name].size();i++) {
        listener[name].at(i)->topicChanged(t);
    }
    
}

juce::StringArray MessageBus::getTopics() {
    StringArray topicList = StringArray();
    
    for(std::map<String,Topic*>::iterator it = topics.begin(); it != topics.end();++it) {
        topicList.add(it->first);
    }
    
    return topicList;
}
