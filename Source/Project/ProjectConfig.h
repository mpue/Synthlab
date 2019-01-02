/*
  ==============================================================================

    ProjectConfig.h
    Created: 31 Dec 2016 11:55:04am
    Author:  Matthias Pueski

  ==============================================================================
*/

#ifndef PROJECTCONFIG_H_INCLUDED
#define PROJECTCONFIG_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

#include "AudioClip.h"
#include "TrackConfig.h"
#include "MixerConfig.h"

#include <vector>

using namespace std;

class ProjectConfig {
    
public:
    ProjectConfig();
    ~ProjectConfig();
    
    vector<AudioClip*> getAudioClips();
    void addClip(AudioClip* clip);
    
    vector<TrackConfig*> getTracks();
    void addTrack(TrackConfig* track);

    MixerConfig* getMixerConfig();
    void setMixerConfig(MixerConfig* config);
    
    void setTracklength(long length);
    long getTrackLength();
    
    void setName(String name);
    String getName();
    
    void setSampleRate(double sampleRate);
    double getSampleRate();

    void setBufferSize(double bufferSize);
    double getBufferSize();
    
    float getTempo();
    void setTempo(float tempo);
    
    ValueTree getConfig();
    void setConfig(ValueTree & config);
    
private:
    
    String name;
    long length;
    double sampleRate;
    int bufferSize;
    long trackLength;
    float tempo;

    vector<AudioClip*> audioClips;
    vector<TrackConfig*> tracks;
    MixerConfig* mixerConfig;
};



#endif  // PROJECTCONFIG_H_INCLUDED
