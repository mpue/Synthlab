/*
  ==============================================================================

    MixerConfig.cpp
    Created: 31 Dec 2016 11:56:14am
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "MixerConfig.h"

MixerConfig::MixerConfig() {
    
}

MixerConfig::~MixerConfig() {
    
}

void MixerConfig::addBusConfig(AuxBusConfig* config) {
    auxBusConfigs.push_back(config);
}