/*
  ==============================================================================

    RegionConfig.h
    Created: 31 Dec 2016 11:55:34am
    Author:  Matthias Pueski

  ==============================================================================
*/

#ifndef REGIONCONFIG_H_INCLUDED
#define REGIONCONFIG_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

#include "AudioClip.h"
#include <vector>

using namespace std;

class RegionConfig {
public:
    RegionConfig();
    ~RegionConfig();
    
    AudioClip* getAudioClip();
    void setAudioClip(AudioClip* clip);
    void addMessage(MidiMessage* message);
    
    ValueTree getConfig();
    
private:
    AudioClip* audioClip;
    MidiMessageSequence* midiSequence;
    
};



#endif  // REGIONCONFIG_H_INCLUDED
