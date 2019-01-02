/*
  ==============================================================================

    Track.h
    Created: 16 Dec 2016 9:40:38am
    Author:  pueskma

  ==============================================================================
*/

#ifndef TRACK_H_INCLUDED
#define TRACK_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"
#include "AudioRegion.h"
#include "MidiRegion.h"
#include "Region.h"
#include <vector>
#include "MultiComponentDragger.h"

//==============================================================================
/*
*/

using namespace std;

class Track : public Component, public ChangeBroadcaster, public ChangeListener
{
public:
    
    enum Type {
        AUDIO,
        MIDI,
        INSTRUMENT
    };
    
    Track(Type type,double sampleRate, MultiComponentDragger* dragger);
    ~Track();
    
	void setZoom(float zoom);
	String getName();
    void setName(String name);
	
    void setGain(float gain);
    float getGain();
	void setVolume(float volume);
	float getVolume();
    float getPan();
    void setPan(float pan);
    
	const float* getReadBuffer(int channel);
    const float getSample(int channel, long sample);
	int getNumSamples();
	void setSelected(bool selected);
	bool isSelected();
	double getMaxLength();
	void addRegion(String refId, File file, double sampleRate);
    void addMidiRegion(double sampleRate, long samplePosition,long regionLength);
    void addRegion(AudioSampleBuffer* source, double sampleRate, long samplePosition, long regionLength);
    void addRegion(String refId, File file, double sampleRate, long samplePosition, long regionLength);
        
    void paint (Graphics&) override;
    void resized() override;

	void setOffset(int offset);
	int getOffset();
    
    
    void setHeight(int height);
    /*
    int getHeight();
     */
    
    void toggleLoopSelection();
    void clearSelection();
    
	AudioSampleBuffer* getBuffer();
  	AudioSampleBuffer* getRecordingBuffer();
    
    void updateMagnitude(int sample, int buffersize, float gainLeft, float gainRight);

    Region* getCurrentRegion(long sample);
    
    void removeSelectedRegions(bool clear);
    void duplicateSelectedRegions();
    void copySelectedRegions();
    void duplicateRegion(Region* region);
    void splitRegion();
    
    void setCurrentMarkerPosition(int position);
    
    int getMidiChannel();
    void setMidiChannel(int channel, bool notify);
    
    vector<Region*> getRegions();
    
    void setTrackLength(long trackLength);
    void setMagnitude(int channel, double magnitude);
    double getMagnitude(int channel);
    
    bool isRecording();
    void setRecording(bool recording);
    
    bool isSolo();
    void setSolo(bool solo);
    
    bool isMute();
    void setMute(bool mute);

    bool isMono();
    void setMono(bool mono);
    
    inline Region* getCurrentRecorder() {
        return currentRegion;
    }
    
    inline void addChangeListener (ChangeListener* listener)  {
        ChangeBroadcaster::addChangeListener(listener);
    }
    
    inline void setInputChannels(int* channels) {
        this->inputChannels[0] = channels[0];
        this->inputChannels[1] = channels[1];
    }
    
    inline int* getInputChannels() {
        return &this->inputChannels[0];
    }
    
    inline void setOutputChannels(int* channels) {
        this->outputChannels[0] = channels[0];
        this->outputChannels[1] = channels[1];
    }
    
    inline int* getOutputChannels() {
        return &this->outputChannels[0];
    }
    
    inline Type getType() {
        return this->type;
    }
    
    inline void setType(Type type) {
        this->type = type;
    }
    
    inline void setMidiInputDevice(String name) {
        this->midiInputDevice = name;
        sendChangeMessage();
    }

    inline void setMidiOutputDevice(String name) {
        this->midiOutputDevice = name;
        sendChangeMessage();
    }

    inline String getMidiInputDevice() {
        return midiInputDevice;
    }
    
    inline String getMidiOutputDevice() {
        return midiOutputDevice;
    }
    
    MidiMessage* getMessage(double time, int sampleNum);
    void addMessage(MidiMessage* message, double time, int sampleNUm);
    
    
    inline void setPlugin(AudioPluginInstance* instance) {
        this->plugin = instance;
    }
    
    inline AudioPluginInstance* getPlugin() {
        return plugin;
    }
    
    inline int getConnectionId() {
        return connectionId;
    }
    
    inline void setConnectionId(unsigned int connectionId) {
        this->connectionId = connectionId;
    }
    
private:

    AudioPluginInstance* plugin = nullptr;
    
	float zoom = 30;
	String name;
	float volume;
	float gain = 1;
    float pan = 0.0f;
	double sampleRate;
	bool selected = false;
	Region* currentRegion = NULL;
	AudioFormatManager* manager;
	vector<Region*> regions;
	long numSamples = 0;
	double maxLength = 600 * this->sampleRate;
	int offset = 0;
	AudioSampleBuffer* audioBuffer;
    MidiBuffer* midiBuffer;
    int markerPosition = 0;
    int bufferSize;
    int midiChannel = 1;
    virtual void changeListenerCallback(ChangeBroadcaster * source) override;
    MultiComponentDragger* dragger = NULL;
    
    bool recording = false;
    bool solo = false;
    bool mute = false;
    bool mono = false;
    
    double magnitudeLeft = 0;
    double magnitudeRight = 0;
    
    int inputChannels[2] = { 0 };
    int outputChannels[2] = { 0, 1 };
    
    String midiInputDevice;
    String midiOutputDevice;
    
    Type type = AUDIO;
    
    int height = Project::DEFAULT_TRACK_HEIGHT;
    
    // plugin connection id
    unsigned int connectionId;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Track)
};


#endif  // TRACK_H_INCLUDED
