/*
  ==============================================================================

    MixerConfig.h
    Created: 31 Dec 2016 11:56:14am
    Author:  Matthias Pueski

  ==============================================================================
*/

#ifndef MIXERCONFIG_H_INCLUDED
#define MIXERCONFIG_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"
#include "AuxBusConfig.h"
#include <vector>

using namespace std;

class MixerConfig {
public:
    MixerConfig();
    ~MixerConfig();
    void addBusConfig(AuxBusConfig* config);
    
private:
    vector<AuxBusConfig*> auxBusConfigs;
};

#endif  // MIXERCONFIG_H_INCLUDED
