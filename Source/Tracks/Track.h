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
#include <memory>
#include <atomic>
#include <limits>
#include "../MultiComponentDragger.h"
#include "../Automation/AutomationModel.h"


//==============================================================================
/*
*/



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
    bool isGridEnabled() const { return gridEnabled; }
    GridSize gridSize;
    void drawGrid(Graphics& g);
    void AddEventListener(EventListener* listener);
    void notifyListeners(Event* e);
    void sendAllNotesOff();
	void setZoom(float zoom);
	float getZoom() const { return zoom; }
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

    // Alt/Option waehrend Bottom-Edge-Resize gedrueckt → Hoehe auf alle anderen
    // Tracks im Navigator spiegeln. Wir registrieren uns als MouseListener auf
    // dem resizer (Track-Ctor), daher kommen diese Events vom Resizer-Child.
    void mouseDrag(const juce::MouseEvent& e) override;

	void setOffset(int offset);
	int getOffset();
    void setHeight(int height);
    void toggleLoopSelection();
    void clearSelection();
    
    MidiBuffer* getMidiBuffer();
	AudioSampleBuffer* getBuffer();
  	AudioSampleBuffer* getRecordingBuffer();

    // Pre-Fader-Send-Quellbuffer (siehe Member-Kommentar). TrackIn schreibt,
    // der Audio-Callback liest. Beide laufen auf dem Audio-Thread.
    AudioSampleBuffer& getPreFaderSendBuffer() { return preFaderSendBuffer; }
    
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
    // Entfernt genau eine Region (per Pointer) sicher — gleiche Teardown-
    // Reihenfolge wie removeSelectedRegions (Recording-Pointer loesen, aus der
    // Dragger-Selektion nehmen, ChangeListener abmelden), damit kein
    // freed-pointer mehr referenziert wird. Wird u.a. vom Auto-Arrange-Undo
    // genutzt, das gezielt seine erzeugten Kopien zuruecknimmt.
    void removeRegion(Region* r, bool clear);
    void duplicateSelectedRegions();

    // Mehrere selektierte AudioRegions auf diesem Track zu einer einzigen
    // verschmelzen: deckt min(offset) .. max(offset+len) ab, summiert
    // Overlaps additiv, fuellt Gaps mit Stille. Markiert die neue Region als
    // bufferMutated (wird beim naechsten Project::save als WAV in $project/edits/
    // persistiert). Gibt die neue Region zurueck, oder nullptr wenn weniger
    // als 2 AudioRegions selektiert sind.
    AudioRegion* mergeSelectedAudioRegions();

    // Analog fuer MIDI/INSTRUMENT-Tracks: alle Events der selektierten
    // MidiRegions werden in eine neue Region kopiert, Timestamps werden in
    // den merged Frame umgerechnet. Originale werden geloescht. Gibt die
    // neue Region zurueck, oder nullptr bei <2 selektierten.
    MidiRegion* mergeSelectedMidiRegions();

    // Type-aware Wrapper: dispatcht auf mergeSelectedAudio/MidiRegions je
    // nach Track-Typ. Liefert true wenn ein Merge stattgefunden hat —
    // Caller (z.B. Consolidate-Menue) faellt bei false auf den Single-Region
    // Pfad zurueck.
    bool mergeSelectedRegions();
    void copySelectedRegions();
    void duplicateRegion(Region* region);
    // Consolidate jede selektierte Region (Region::selected || dragger).
    // Wird vom Menu-Handler ueber alle Tracks dispatched — selektierte
    // Regionen koennen auf beliebig vielen Tracks liegen.
    void consolidateSelectedRegions();
    // Reverse jede selektierte AudioRegion auf diesem Track.
    void reverseSelectedAudioRegions();
    // Paste every compatible region from Session's clipboard onto this track,
    // starting at startSampleOffset. Audio-only into AUDIO tracks, MIDI-only
    // into MIDI/INSTRUMENT tracks; incompatible entries are skipped.
    void pasteRegions(long startSampleOffset);
    // Paste a single region (typically from the clipboard) onto this track at
    // targetSampleOffset. Returns the placed region or nullptr if the source
    // type doesn't match this track's type.
    Region* pasteRegion(Region* source, long targetSampleOffset);
    void splitRegion();

    // Rendert die uebergebene MIDI-Region offline durch das Instrument-Plugin.
    // Gibt einen frisch allozierten Stereo-AudioSampleBuffer in Region-Laenge
    // zurueck (Caller wird Eigentuemer); nullptr bei Fehler (kein Plugin,
    // keine MIDI-Region, ungueltige Laenge). Source-Region bleibt unangetastet.
    juce::AudioSampleBuffer* bounceRegionToBuffer(Region* sourceRegion);

    void attachRegion(Region* r);
    void detachRegion(Region* r);

    MultiComponentDragger* getDragger() { return dragger; }
    
    void setSamplePosition(int position, bool dispatchEvents = true);
    int getSamplePosition();
    void forcePositionReset();  // invalidates samplePosition so next setSamplePosition always dispatches MIDI

    void setCurrentMarkerPosition(int pos);

    int getMidiChannel();
    void setMidiChannel(int channel, bool notify);
    
    std::vector<Region*> getRegions();
    
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

    Region* getRecordingRegion() const { return recordingRegion; }
    void setRecordingRegion(Region* r) { recordingRegion = r; }
    void clearRecordingRegionIfMatches(Region* r) {
        if (recordingRegion == r) recordingRegion = nullptr;
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

    // ----- Bus-Automation (dedizierte Lane) ------------------------------------
    // Zweites, von der Plugin-Automation unabhaengiges Overlay. Automatisiert die
    // Bus-Anbindung dieses Tracks: Send-Staerke auf einen Bus sowie das
    // Volume/Pan/Mute des Bus-Channels. Die Lanes leben im selben automationModel
    // wie die Plugin-Lanes, nur mit gescopter paramId "bus:<busModuleIndex>:<kind>".
    enum class BusParam { Send, Volume, Pan, Mute };
    static const char* busParamSuffix(BusParam p);             // "send"|"volume"|"pan"|"mute"
    static juce::String busLaneParamId(long busModuleIndex, BusParam p);

    void setBusAutomationVisible(bool visible);
    bool isBusAutomationVisible() const { return busAutomationVisible; }

    // Bindet das Bus-Overlay an die Lane fuer (busModuleIndex, kind). Legt die
    // Lane idempotent an, falls noch nicht vorhanden.
    void setBusAutomationTarget(long busModuleIndex, BusParam kind);

    long     getBusAutomationCurrentBusIndex() const { return busAutomationCurrentBusIndex; }
    BusParam getBusAutomationCurrentParam()    const { return busAutomationCurrentParam; }
    bool     hasBusAutomationTarget()          const { return busAutomationCurrentBusIndex >= 0; }

    AutomationLane* getBusAutomationLane() const { return busAutomationLane.get(); }

    // Verfuegbare Buses fuer die Bus-Automation-Auswahl als (busModuleIndex, name).
    // Kapselt den Mixer-/Editor-Zugriff, damit TrackControl keine schweren Header
    // ziehen muss. Leer, wenn kein Editor/Mixer erreichbar oder keine Buses da.
    std::vector<std::pair<long, juce::String>> getBusList() const;

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

    // ----- Bus-Sends ---------------------------------------------------------
    // Pro Track 0..N Sends auf BusOut-Module. Die Routing-Logik lebt nicht im
    // Patch-Graph — der Audio-Callback liest diese Liste und mischt das Track-
    // signal pro Send mit `amount` in den jeweiligen BusOut::mixBuffer.
    //
    // Tap-Punkt: PRE-FADER. Fuer AUDIO-Tracks ist die Quelle der von TrackIn
    // gefuellte preFaderSendBuffer (Pan ja, aber ohne Volume-Fader und ohne
    // Mute/Solo) — der Send bleibt damit unabhaengig vom Kanal-Fader und vom
    // Mute (Cue-/Kopfhoerer-Mix-Verhalten). Nicht-Audio-Strips (Instrument)
    // greifen weiterhin post-fader am Output-Modul-Eingang ab.
    //
    // busModuleIndex referenziert BusOut::getIndex() (= Module::getIndex(),
    // stabil ueber Save/Load), nicht den volatilen Mixer-Channel-Index.
    struct Send {
        long  busModuleIndex = -1;
        float amount         = 1.0f; // 0..1 linear (pre-fader tap)
    };

    const std::vector<Send>& getSends() const { return sends; }
    void addSend(long busModuleIndex, float amount = 1.0f) {
        sends.push_back({ busModuleIndex, amount });
        sendChangeMessage();
    }
    void removeSend(int idx) {
        if (idx >= 0 && idx < (int) sends.size()) {
            sends.erase(sends.begin() + idx);
            sendChangeMessage();
        }
    }
    void setSendAmount(int idx, float amount) {
        if (idx >= 0 && idx < (int) sends.size()) {
            sends[idx].amount = amount;
            sendChangeMessage();
        }
    }
    void setSendBus(int idx, long busModuleIndex) {
        if (idx >= 0 && idx < (int) sends.size()) {
            sends[idx].busModuleIndex = busModuleIndex;
            sendChangeMessage();
        }
    }

private:

    void dispatchMidiMessages(const std::vector<juce::MidiMessage*>& messages);

    // Audio-Thread: wendet alle "bus:*"-Lanes des automationModel direkt auf ihre
    // Ziele an (Send.amount bzw. Mixer::Channel(BUS) Volume/Pan/Mute).
    void applyBusAutomationAt(int position);

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
	std::vector<Region*> regions;
	long numSamples = 0;
	double maxLength = 600 * this->sampleRate;
	int offset = 0;
	AudioSampleBuffer* audioBuffer;
    // Pre-Fader-Quellsignal fuer Bus-Sends: pro Block von TrackIn::process
    // befuellt (nur AUDIO-Tracks) mit dem Tracksignal NACH Pan, aber VOR
    // Volume-Fader und VOR Mute/Solo — so bleibt der Send unabhaengig vom
    // Kanal-Fader und vom Mute (Cue-/Kopfhoerer-Mix-Verhalten). Wird vom
    // Audio-Callback (MainComponent::getNextAudioBlock, BounceThread) gelesen.
    AudioSampleBuffer preFaderSendBuffer;
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
    Region* recordingRegion = nullptr;
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

    // Dediziertes Bus-Automation-Overlay (unabhaengig von automationLane). Bindet
    // an Lanes mit paramId "bus:<idx>:<kind>" im selben automationModel.
    std::unique_ptr<AutomationLane> busAutomationLane;
    bool     busAutomationVisible        = false;
    long     busAutomationCurrentBusIndex = -1;
    BusParam busAutomationCurrentParam    = BusParam::Send;

    // Mirror der FX-Plugin-Pointer aus dem MasterChannelPanel. Wird vom Panel
    // gepflegt (insertFX/removeFX/applyFXChainState/reconstructFXChainFromGraph).
    PluginModule* fxPlugins[NUM_FX_SLOTS] = {};

    // Bus-Sends dieses Tracks. Lesen vom Audio-Thread, Mutation nur vom UI-
    // Thread (Track-Strip "+"-Button, Dropdown / Knob / Remove). Der Audio-
    // Thread liest size() + amount/index ohne Lock — wenn der User waehrend
    // eines Blocks einen Send loescht, kann ein Block lang ein "phantom" Send
    // gemischt werden; das hoert kein Mensch und ist die Standardloesung in
    // diesem Code (siehe FX-Chain-Reads).
    std::vector<Send> sends;

    // Datenmodell fuer alle Lanes dieses Tracks. Die Lane des aktuell im UI
    // gewaehlten Parameters bindet `automationLane` an sich; alle anderen
    // bleiben in `automationModel` und werden bei Wiedergabe trotzdem
    // angewandt.
    synthlab::automation::Model automationModel;

    // Vorberechneter Cache: gescoped paramId -> AudioProcessorParameter*.
    // Wird vom Audio-Thread alloc-frei gelesen, vom UI-Thread bei Plugin-/
    // FX-Wechsel via rebuildAutomationParamIdCache() neu gebaut.
    //
    // Threading: Der Audio-Thread schreibt waehrend setSamplePosition()
    // nur in pendingValue/hasPending — er ruft NICHT setValue() am Plugin.
    // Der Message-Thread (TrackNavigator-Timer -> applyPendingAutomation())
    // liest die Atomics und schickt die Werte erst hier ans Plugin.
    // Hintergrund: VST3 IEditController::setParamNormalized ist per Spec
    // message-thread, viele Plugins triggern aus setValue/setParamNormalized
    // synchron ihre UI-Listener (z.B. MainUI::parameterChanged) — das crasht,
    // sobald das Plugin-Fenster offen ist und der Aufruf vom Audio-Thread
    // kommt.
    struct CachedAutomationParam {
        juce::String                   scopedId;   // "instr:foo", "fx0:bar"
        juce::AudioProcessorParameter* parameter = nullptr;
        std::atomic<float>             pendingValue { 0.0f };
        std::atomic<bool>              hasPending   { false };
        float                          lastApplied  = std::numeric_limits<float>::quiet_NaN();
    };
    // unique_ptr, weil CachedAutomationParam wegen der Atomics weder
    // copy- noch move-konstruierbar ist und ein std::vector also nicht direkt
    // damit umgehen kann.
    std::vector<std::unique_ptr<CachedAutomationParam>> cachedAutomationParams;

public:
    // Vom Message-Thread aufzurufen (TrackNavigator-Timer). Schiebt die vom
    // Audio-Thread bereitgestellten Pending-Werte ins Plugin — message-thread-
    // safe.
    void applyPendingAutomation();
private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Track)
};


#endif  // TRACK_H_INCLUDED
