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
#include <set>
#include "../MultiComponentDragger.h"
#include "../Automation/AutomationModel.h"


//==============================================================================
/*
*/

using namespace std;

class TrackNavigator;
class PluginModule;
class AutomationLane;

class Track : public Component, public ChangeBroadcaster, public ChangeListener
{
public:
    enum GridSize {
        WHOLE_NOTE,
        HALF_NOTE,
        QUARTER_NOTE,
        EIGHTH_NOTE,
        SIXTEENTH_NOTE
    };
    
    enum Type {
        AUDIO,
        MIDI,
        INSTRUMENT
    };
    
    Track(Type type,double sampleRate, MultiComponentDragger* dragger);
    ~Track();
    
    long getGridSizeInSamples() const;
    int getGridSizeInPixels() const;
    GridSize gridSize;
    void drawGrid(Graphics& g);
    void AddEventListener(EventListener* listener);
    void notifyListeners(Event* e);
    void sendAllNotesOff();
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
    AudioRegion* addRegion(AudioSampleBuffer* source, double sampleRate, long samplePosition, long regionLength);
    void addRegion(String refId, File file, double sampleRate, long samplePosition, long regionLength);
        
    void paint (Graphics&) override;
    void resized() override;

	void setOffset(int offset);
	int getOffset();
    void setHeight(int height);
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

    // Regenbogen-Farbe für diesen Track. Basis ist der Index des Tracks
    // im Navigator — alle gleichzeitig sichtbaren Tracks bekommen dadurch
    // unterschiedliche Hues. Regionen, die auf diesem Track liegen, fragen
    // dieselbe Farbe ab, sodass der visuelle Bezug erhalten bleibt.
    juce::Colour getTrackColour() const;

    juce::String getInstrumentName() {
        return instrumentName;
    }

    void setInstrumentName(juce::String name) {
        instrumentName = name;
	}

    void updateMagnitude(int sample, int buffersize, float gainLeft, float gainRight);

    Region* getRegionAt(long sample);
    Region* getCurrentRegion();

    // Overlap prevention. `moving` is the region being placed/resized
    // (excluded from the check); pass nullptr when inserting a new region.
    long clampOffsetForRegion(Region* moving, long desiredOffset, long length) const;
    long clampLengthForRegion(Region* moving, long offset, long desiredLength) const;
    long findNextFreeOffset(Region* moving, long desiredOffset, long length) const;

    void removeSelectedRegions(bool clear);
    void duplicateSelectedRegions();
    void copySelectedRegions();
    void duplicateRegion(Region* region);
    // Paste every compatible region from Session's clipboard onto this track,
    // starting at startSampleOffset. Audio-only into AUDIO tracks, MIDI-only
    // into MIDI/INSTRUMENT tracks; incompatible entries are skipped.
    void pasteRegions(long startSampleOffset);
    // Paste a single region (typically from the clipboard) onto this track at
    // targetSampleOffset. Returns the placed region or nullptr if the source
    // type doesn't match this track's type.
    Region* pasteRegion(Region* source, long targetSampleOffset);
    void splitRegion();

    void attachRegion(Region* r);
    void detachRegion(Region* r);

    MultiComponentDragger* getDragger() { return dragger; }
    
    void setSamplePosition(int position, bool dispatchEvents = true);
    int getSamplePosition();
    void forcePositionReset();  // invalidates samplePosition so next setSamplePosition always dispatches MIDI

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

    void setPluginModule(PluginModule* module) {
        this->pluginModule = module;
        rebuildAutomationParamIdCache();
	}

    // Wird vom Track gepflegt, damit setSamplePosition() im Audio-Thread keine
    // juce::String-Allokationen mehr braucht (#7).
    void rebuildAutomationParamIdCache();

    PluginModule* getPluginModule() {
        return this->pluginModule;
	}

    void setBPM(double bpm) {
		this->bpm = bpm;
    }

    // ----- Lane-Automation -----------------------------------------------------
    // Overlay zum Zeichnen einer Parameter-Automation. Wird vom TrackControl
    // per Toggle ein-/ausgeblendet; im aktiven Zustand fängt es Mausklicks ab.
    void setAutomationVisible(bool visible);
    bool isAutomationVisible() const { return automationVisible; }

    void setAutomationParameterIndex(int idx);
    int  getAutomationParameterIndex() const { return automationParamIndex; }

    AutomationLane* getAutomationLane() const { return automationLane.get(); }

    // ----- Multi-Lane Automation (Phase 1+3 Wiring) ----------------------------
    // Pro Plugin-Parameter eine Lane oder keine. Stabile Uuids ueber Save/Load.
    synthlab::automation::Model&       getAutomationModel()       { return automationModel; }
    const synthlab::automation::Model& getAutomationModel() const { return automationModel; }

    // ParameterId fuer den Plugin-Parameter an Listen-Index `idx` *im
    // Instrument-Plugin*. Stabil ueber Save/Load.
    juce::String automationParameterIdFor(int idx) const;

    // ----- Multi-Plugin Automation -------------------------------------------
    // Ein Track hat heute bis zu vier FX-Slots zusaetzlich zum Instrument.
    // Slot-Namen ("instr", "fx0".."fx3") fungieren als Scope, damit
    // gleichnamige Parameter unterschiedlicher Plugins kollisionsfrei
    // automatisierbar sind.
    static constexpr int NUM_FX_SLOTS = 4;
    static const char*   kInstrumentSlotName;  // "instr"
    static juce::String  fxSlotName(int idx) { return "fx" + juce::String(idx); }

    // Wird vom MasterChannelPanel nach insertFX/removeFX/Reconstruct gerufen.
    void setFxPlugin(int slot, PluginModule* pm);
    PluginModule* getFxPlugin(int slot) const
    {
        return (slot >= 0 && slot < NUM_FX_SLOTS) ? fxPlugins[slot] : nullptr;
    }

    // Liefert alle automatisierbaren Plugin-Slots dieses Tracks
    // (Instrument + belegte FX-Slots) als (Slot-Name, PluginModule*).
    std::vector<std::pair<juce::String, PluginModule*>> getAutomationTargets() const;

    // Bindet die UI-Lane an die Lane fuer (slot, paramIdInPlugin). Ersatz fuer
    // setAutomationParameterIndex, der weiterhin den Instrument-Slot verwaltet.
    void setAutomationTarget(const juce::String& slot, const juce::String& paramIdInPlugin);

    // Aktuell im UI gewaehlte Lane-Identifikation, fuer die TrackControl-
    // Wiederherstellung beim Repaint/Refresh.
    juce::String getAutomationCurrentSlot()   const { return automationCurrentSlot; }
    juce::String getAutomationCurrentParamId() const { return automationCurrentParamId; }

private:

    void dispatchMidiMessages(const std::vector<juce::MidiMessage*>& messages);

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

    std::multiset<int> activeNoteNumbers;
    MidiRegion* lastActiveRegion = nullptr;
    
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

	PluginModule* pluginModule = nullptr;

    juce::Array<juce::PropertyComponent*> properties;
  
    TrackNavigator* navigator;

	bool gridEnabled = true;
    double bpm = 120.0;

    std::unique_ptr<AutomationLane> automationLane;
    bool automationVisible = false;
    int  automationParamIndex = -1;
    juce::String automationCurrentParamId; // gescoped: "slot:paramId"
    juce::String automationCurrentSlot;    // "instr" | "fx0"..

    // Mirror der FX-Plugin-Pointer aus dem MasterChannelPanel. Wird vom Panel
    // gepflegt (insertFX/removeFX/applyFXChainState/reconstructFXChainFromGraph).
    PluginModule* fxPlugins[NUM_FX_SLOTS] = {};

    // Datenmodell fuer alle Lanes dieses Tracks. Die Lane des aktuell im UI
    // gewaehlten Parameters bindet `automationLane` an sich; alle anderen
    // bleiben in `automationModel` und werden bei Wiedergabe trotzdem
    // angewandt.
    synthlab::automation::Model automationModel;

    // Vorberechneter Cache: gescoped paramId -> AudioProcessorParameter*.
    // Wird vom Audio-Thread alloc-frei gelesen, vom UI-Thread bei Plugin-/
    // FX-Wechsel via rebuildAutomationParamIdCache() neu gebaut.
    struct CachedAutomationParam {
        juce::String                   scopedId;   // "instr:foo", "fx0:bar"
        juce::AudioProcessorParameter* parameter = nullptr;
    };
    std::vector<CachedAutomationParam> cachedAutomationParams;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Track)
};


#endif  // TRACK_H_INCLUDED
