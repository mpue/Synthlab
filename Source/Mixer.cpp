//
//  Mixer.cpp
//  Synthlab - App
//
//  Created by Matthias Pueski on 23.04.18.
//  Copyright © 2018 Pueski. All rights reserved.
//

#include "Mixer.h"
#include <vector>

Mixer::Mixer() {
    
}

Mixer::~Mixer() {
    for (std::vector<Channel*>::iterator it = channels.begin();it != channels.end();) {
        delete (*it);
        it = channels.erase(it);
    }
}

void Mixer::addChannel(Mixer::Channel *channel) {
    channels.push_back(channel);
}

std::vector<Mixer::Channel*>& Mixer::getChannels() {
    return channels;
}

int Mixer::getNumInputs() {
    
    int numinputs = 0;
    
    for (int i = 0; i < channels.size();i++) {
        if (channels.at(i)->channelType == Channel::Type::IN) {
            numinputs++;
        }
    }
    
    return numinputs;
}

int Mixer::getNumOutputs() {
    
    int numoutputs = 0;
    
    for (int i = 0; i < channels.size();i++) {
        if (channels.at(i)->channelType == Channel::Type::OUT) {
            numoutputs++;
        }
    }
    
    return numoutputs;
}


int Mixer::getNumAuxBusses() {
    
    int busses = 0;
    
    for (int i = 0; i < channels.size();i++) {
        if (channels.at(i)->channelType == Channel::Type::AUX) {
            busses++;
        }
    }
    
    return busses;
}

Mixer::Channel* Mixer::getChannel(Mixer::Channel::Type type, int index) {
    for (int i = 0; i < channels.size();i++) {
        if (channels.at(i)->channelType == type && channels.at(i)->index == index) {
            return channels.at(i);
        }
    }
    return nullptr;
}
