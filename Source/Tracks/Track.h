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
#include "../MultiComponentDragger.h"


//==============================================================================
/*
*/

using namespace std;

class TrackNavigator;

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
    

    void AddEventListener(EventListener* listener);
    void notifyListeners(Event* e);
	void setZoom(float zoom);
	String getName();
    void setName(String name);
	
    void setGain(float gain);
    float getGain();
	void setVolume(float volume);
	float getVolume();
    float getPan();
    void setPan(float pan);
    void setBufferSize(int bufferSize) {
        this->bufferSize = bufferSize;
	}
	const float* getReadBuffer(int channel);
    const float getSample(int channel, long sample);
	int getNumSamples();
	void setSelected(bool selected);
	bool isSelected();
	double getMaxLength();
	Region* addRegion(String refId, File file, double sampleRate, long samplePosition);
    void addRegion(String refId, File file, double sampleRate);
    MidiRegion* addMidiRegion(double sampleRate, long samplePosition,long regionLength);
    void addMidiRegion(juce::MidiBuffer* source, double sampleRate, long samplePosition);
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
    
    MidiBuffer* getMidiBuffer();

	AudioSampleBuffer* getBuffer();
  	AudioSampleBuffer* getRecordingBuffer();
    
    void setNavigator(TrackNavigator* navigator) {
        this->navigator = navigator;
    }

    TrackNavigator* getNavigator() {
		return this->navigator;
	}

    juce::String getInstrumentName() {
        return instrumentName;
    }

    void setInstrumentName(juce::String name) {
        instrumentName = name;
	}

    void updateMagnitude(int sample, int buffersize, float gainLeft, float gainRight);

    Region* getRegionAt(long sample);
    Region* getCurrentRegion();

    void removeSelectedRegions(bool clear);
    void duplicateSelectedRegions();
    void copySelectedRegions();
    void duplicateRegion(Region* region);
    void splitRegion();
    
    void setSamplePosition(int position);
    int getSamplePosition();

    void setCurrentMarkerPosition(int pos);

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
    
    juce::Array<juce::PropertyComponent*>& getProperties();
    void createProperties();

    void setSample(int channel, int sampleNum, float sample);

private:

    juce::StringArray getTypeNames() const
    {
        return { "AUDIO", "MIDI", "INSTRUMENT" };
    }

    juce::Array<var> getTypeValues() const
    {
        return { Type::AUDIO, Type::MIDI, Type::INSTRUMENT };
    }

    struct TypeListener : juce::Value::Listener
    {
        TypeListener(juce::Value& v, Track* t) : track(t), value(v) { value.addListener(this); }
        ~TypeListener() {}  // no need to remove the listener

        void valueChanged(juce::Value& value) override {
            track->setType((Track::Type)value.toString().getIntValue());
        }
        Track* track;
        juce::Value value;
    };


    struct NameListener : juce::Value::Listener
    {
        NameListener(juce::Value& v, Track* t) : track(t), value(v) { value.addListener(this); }
        ~NameListener() {}  // no need to remove the listener

        void valueChanged(juce::Value& value) override {
            track->setName(value.toString());
        }
        Track* track;
        juce::Value value;
    };

    struct RecordingListener : juce::Value::Listener
    {
        RecordingListener(juce::Value& v, Track* t) : track(t), value(v) { value.addListener(this); }
        ~RecordingListener() {}  // no need to remove the listener

        void valueChanged(juce::Value& value) override {
            track->setRecording(value.toString().getIntValue() > 0);
        }
        Track* track;
        juce::Value value;
    };


    struct MIdiChannelListener : juce::Value::Listener
    {
        MIdiChannelListener(juce::Value& v, Track* t) : track(t), value(v) { value.addListener(this); }
        ~MIdiChannelListener() {}  // no need to remove the listener

        void valueChanged(juce::Value& value) override {
            track->setMidiChannel(value.toString().getIntValue(), false);
        }
        Track* track;
        juce::Value value;
    };

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
    int samplePosition = 0;
    int bufferSize;
    int midiChannel = 1;
    virtual void changeListenerCallback(ChangeBroadcaster * source) override;
    MultiComponentDragger* dragger = NULL;
    
    ResizableEdgeComponent* resizer;
    ResizeConstrainer* constrainer;

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
    
	String instrumentName;

    Type type = AUDIO;
    
    int height = Project::DEFAULT_TRACK_HEIGHT;
    
    // plugin connection id
    unsigned int connectionId;

    NameListener* nameListener;
    juce::Value* nameValue;
    juce::PropertyComponent* nameProp;

    TypeListener* typeListener;
    juce::Value* typeValue;
    
    juce::PropertyComponent* typeProp;
    std::vector<EventListener*> listeners;
    
    RecordingListener* recordingListener;
    juce::Value* recordingValue;
    juce::PropertyComponent* recordingProp;

    MIdiChannelListener* channelListener;
    juce::Value* channelValue;
	juce::PropertyComponent* channelProp;


    juce::Array<juce::PropertyComponent*> properties;
  
    TrackNavigator* navigator;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Track)
};


#endif  // TRACK_H_INCLUDED
