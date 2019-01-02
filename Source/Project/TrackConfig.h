/*
  ==============================================================================

    TrackConfig.h
    Created: 31 Dec 2016 11:55:18am
    Author:  Matthias Pueski

  ==============================================================================
*/

#ifndef TRACKCONFIG_H_INCLUDED
#define TRACKCONFIG_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

#include "RegionConfig.h"
#include "EffectConfig.h"
#include "InstrumentConfig.h"
#include "AuxSendConfig.h"

#include <vector>

using namespace std;

class TrackConfig {
public:
    TrackConfig();
    ~TrackConfig();
    
    // the possible regins a track can have
    void addRegion(RegionConfig* region);
    // the asscociated fx chain, we need to take care of the order!
    void addEffect(EffectConfig* effect);
    // the aux send channels of a track, order is also important
    void addAuxSend(AuxSendConfig* aux);
    // the associated instrument if this is an instrumen track
    void setInstrument(InstrumentConfig* instrument);
    
    vector<RegionConfig*> getRegions();
    
    
    String getName();
    void setName(String name);
    
    void setGain(float gain);
    
    void setVolume(float volume);
    float getVolume();

    float getPan();
    void setPan(float pan);
    
    void setMidiChannel(int channel);
    float getMidiChannel();
    
    void setInputChannels(int* channels);
    int* getInputChannels();
    
    void setOutputChannels(int* channels);
    int* getOutputChannels();
    
    String getType();
    void setType(String type);
    
    String getMidiInputDevice();
    void setMidiInputDevice(String name);
    
    String getMidiOutputDevice();
    void setMidiOutputDevice(String name);
    
    bool isSolo();
    void setSolo(bool solo);
    
    bool isMute();
    void setMute(bool mute);
    
    bool isMono();
    void setMono(bool mono);
    
    void setHeight(int height);
    int getHeight();
    
    ValueTree getConfig();
    
private:

    String name;
    
    float volume;
    float gain = 1;
    float pan = 0.0f;
    
    int midiChannel = 1;

    bool solo = false;
    bool mute = false;
    bool mono = false;
    
    int inputChannels[2] = { 0 };
    int outputChannels[2] = { 0 };
    
    String midiInputDevice;
    String midiOutputDevice;
    
    String type;
    vector<RegionConfig*> regions;
    vector<EffectConfig*> effects;
    vector<AuxSendConfig*> auxSendConfigs;
    InstrumentConfig* instrument = NULL;
    
    int height = 0;
    
};

#endif  // TRACKCONFIG_H_INCLUDED
