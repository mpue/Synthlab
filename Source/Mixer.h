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


class Mixer {
  
public:
    
    class Channel {
        
    public:
        enum Type {
            IN,
            OUT,
            AUX
        };
        
        float volume;
        float pan;
        float magnitudeLeft = 0;
        float magnitudeRight = 0;
        bool mute = false;
        bool solo = false;
        juce::String name;
        Type channelType;
        int index;
    };
    
    Mixer();
    ~Mixer();
    
    std::vector<Channel*>& getChannels();
    void addChannel(Channel* channel);
    void removeChannel(int index);
    void removeAllChannels();
    
    Channel* getChannel(Mixer::Channel::Type type, int index);
    
    
    
    int getNumOutputs();
    int getNumInputs();
    int getNumAuxBusses();
    
private:
    std::vector<Channel*> channels;
};

#endif /* Mixer_hpp */
