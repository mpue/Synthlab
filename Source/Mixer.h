//
//  Mixer.hpp
//  Synthlab - App
//
//  Created by Matthias Pueski on 23.04.18.
//  Copyright © 2018 Pueski. All rights reserved.
//

#ifndef Mixer_h
#define Mixer_h

#include "../JuceLibraryCode/JuceHeader.h"
#include "Tracks/Track.h"

class 		Mixer : public ChangeListener, public ChangeBroadcaster {
    
public:

    static Mixer* getInstance() {
        if (instance == NULL) {
            instance = new Mixer();
        }
        return instance;
    }
    
    static void destroy() {
        delete instance;
    }
    
    class Channel {
        
    public:
        enum Type {
            IN,
            OUT,
            AUX
        };
        
        double volume;
        double pan;
        float magnitudeLeft = 0;
        float magnitudeRight = 0;
        bool mute = false;
        bool solo = false;
        juce::String name;
        Type channelType;
        int index;
    };
    

    inline void addTrack(Track* track) {
        this->tracks.push_back(track);
        track->addChangeListener(this);
        sendChangeMessage();
    }
    
    vector<Track*> getTracks() {
        return this->tracks;
    }
    
    inline virtual void changeListenerCallback (ChangeBroadcaster* source) override {
        
        if (Track* t = dynamic_cast<Track*>(source)) {
            lastModified = t;
            sendChangeMessage();
        }
        
    }
    
    Track* getLastModifiedTrack() {
        return this->lastModified;
    }
    
    
    inline void addChangeListener (ChangeListener* listener) {
        ChangeBroadcaster::addChangeListener(listener);
    }
    
    std::vector<Channel*>& getChannels();
    void addChannel(Channel* channel);
    void removeChannel(int index);
    void removeAllChannels();
    
    Channel* getChannel(Mixer::Channel::Type type, int index);
    
    
    
    int getNumOutputs();
    int getNumInputs();
    int getNumAuxBusses();
    
    void setAvailableInputChannelNames(StringArray channels) {
        this->availableInputChannels = channels;
    }
    
    StringArray getInputChannels() {
        return this->availableInputChannels;
    }
    
    void setAvailableOutputChannelNames(StringArray channels) {
        this->availableOutputChannels = channels;
    }
    
    StringArray getOutputChannels() {
        return this->availableOutputChannels;
    }
    
    vector<String> getMidiInputs() {
        return midiInputs;
    }
    
    vector<String> getMidiOutputs() {
        return midiOutputs;
    }
    
    void clearMidiInputs() {
        this->midiInputs.clear();
    }
    
    void clearMidiOutputs() {
        this->midiOutputs.clear();
    }
    
    void addMidiInput(String name) {
        this->midiInputs.push_back(name);
    }
    
    void addMidiOutput(String name) {
        this->midiOutputs.push_back(name);
    }
    
private:
    
    Mixer();
    ~Mixer();
    
    static Mixer* instance;
    
    vector<Track*> tracks;
    Track* lastModified = NULL;
    
    std::vector<Channel*> channels;
    
    StringArray availableInputChannels;
    StringArray availableOutputChannels;
    
    vector<String> midiInputs;
    vector<String> midiOutputs;
};

#endif /* Mixer_hpp */
