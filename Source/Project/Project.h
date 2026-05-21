/*
  ==============================================================================

    Project.h
    Created: 21 Dec 2016 7:45:32pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#ifndef PROJECT_H_INCLUDED
#define PROJECT_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

#include "ProjectConfig.h"
#include "ProjectSnapshot.h"
#include "../MainTabbedComponent.h"
#include "../AudioEngine/Sampler.h"
#include "../PropertyView.h"
#include "../Module.h"
#include "../CustomLookAndFeel.h"

#include "../ElegantDarkLookAndFeel.h"

#include <vector>
#include <map>

using namespace std;

class MidiProcessor;

class Project : public ChangeBroadcaster {
    
public:
    
    static Project* getInstance() {
        // Re-Erzeugen nach Project::destroy() ist ein Bug (typisch beim
        // Tear-Down): ein frisch konstruiertes Project ist nur halb-init und
        // crasht beim ersten Method-Aufruf. Wenn destroyed schon true ist,
        // geben wir nullptr zurueck und der Aufrufer muss damit umgehen
        // koennen — alle cleanUp-Pfade tun das (auto* p = ...; if (p) ...).
        if (instance == nullptr) {
            if (destroyed) return nullptr;
            instance = new Project();
        }
        return instance;
    }
    
    enum AppMode {
        STUDIO,
        PLAYER
    };

    static void destroy();
    
    juce::UndoManager* getUndoManager();
    juce::StringArray& getRecentFiles();
    void loadRecentFileList();
    void addRecentFile(juce::String path);
    
    void setSupplemental(MainTabbedComponent* component) ;
    MainTabbedComponent* getSupplemental();
    
    ElegantDarkLookAndFeel* getLookAndFeel();

    std::vector<juce::DialogWindow*>& getOpenWindows();
    
    void setDefaultSampler(Sampler* sampler);
    Sampler* getDefaultSampler();

    // Wired by MainComponent at startup. Used by TriggerScriptModule (and any
    // future module that wants to invoke a JS macro) — we route through the
    // existing JavaScriptEditorComponent::runFile so the same Duktape pipeline
    // executes, output lands in the same panel, errors bubble the same way.
    // Raw pointer, no ownership; the JS editor outlives modules in normal
    // teardown order (Project is destroyed last).
    void setJavaScriptEditor(class JavaScriptEditorComponent* ed) { jsEditor = ed; }
    class JavaScriptEditorComponent* getJavaScriptEditor() const  { return jsEditor; }
    
    void setRoot(Module* root);
    Module* getRoot();
    
    juce::Component* getMain();
    void setMain(juce::Component* c);
    
    void setPropertyView(PropertyView* view);
    PropertyView* getPropertyView();
    
    ApplicationCommandManager* getCommandManager();
    void setCommandManager(ApplicationCommandManager* commandManager);
    
    AudioDeviceManager* getAudioDeviceManager() {
		return deviceManager;
    }

    void setAudioDeviceManager(AudioDeviceManager* deviceManager) {
		this->deviceManager = deviceManager;
    }

    AppMode getAppMode();
    
    String getCurrentFilePath();
    void setCurrentFilePath(String path);
    
    String getCurrentFileName();
    void setCurrentFileName(String path);
    
    bool isNewFile();
    void setNewFile(bool isNew);
    
    bool isDirty();
    void setDirty(bool dirty);
    
    void setTracklength(long length) {
        this->tracklength = length;
        sendChangeMessage();
    };
    
    long getTrackLength() {
        return this->tracklength;
    }

    void setName(String name) {
        this->name = name;
        sendChangeMessage();
    }
    
    String getName() {
        return name;
    }
    
    void setSampleRate(double sampleRate) {
        this->sampleRate = sampleRate;
    }
    
    // TODO: where should this be set???
    double getSampleRate() {
		return this->getAudioDeviceManager()->getCurrentAudioDevice()->getCurrentSampleRate(); 
    }
    
    void setBufferSize(int bufferSize) {
        this->bufferSize = bufferSize;
    }
    
    int getBufferSize() {
        return bufferSize;
    }
    
    inline void addChangeListener (ChangeListener* listener) {
        ChangeBroadcaster::addChangeListener(listener);
    }

    ProjectConfig* getConfig() {
        return config;
    }
    
    void createNew(String _name) {
        this->name = _name;
        
        if (config != NULL) {
            delete config;
        }
        
        config = new ProjectConfig();
        clear();
    }
    
    float getTempo() {
        return tempo;
    }

    void setTempo(float tempo) {
        if (this->tempo == tempo) return;
        this->tempo = tempo;
        // Listener (insb. TrackNavigator) bekommen so Bescheid, dass das
        // Tempo neu ist und ggf. AudioRegions mit followTempo=true gestretcht
        // werden muessen.
        sendChangeMessage();
    }

    bool isSnapToGridEnabled() const {
        return snapToGridEnabled;
    }

    void setSnapToGridEnabled(bool enabled) {
        snapToGridEnabled = enabled;
        sendChangeMessage();
    }

    // Round a sample position to the nearest grid step using the project
    // tempo. Below the high-zoom threshold the grid is one bar (4 beats);
    // at zoom >= 100 the grid switches to a quarter-bar (= one beat in 4/4),
    // because at that resolution per-bar snapping feels too coarse. When
    // fineMode is true (Shift held while dragging), the step becomes 1/4 of
    // the otherwise-active grid for finer placement.
    // Returns the input unchanged if tempo or sample rate aren't available.
    long snapSampleToGrid(long sample, float zoom, bool fineMode = false) {
        const double sr = getSampleRate();
        if (tempo <= 0.0f || sr <= 0.0)
            return sample;
        double beatsPerStep = (zoom >= 100.0f) ? 1.0 : 4.0;
        if (fineMode)
            beatsPerStep *= 0.25;
        const double samplesPerStep = (60.0 / tempo) * beatsPerStep * sr;
        if (samplesPerStep <= 0.0)
            return sample;
        const long step = (long)std::round((double)sample / samplesPerStep);
        return (long)std::round((double)step * samplesPerStep);
    }
    
    // Erfuellt Regel B: liest GENAU EINMAL aus dem Snapshot. publishSnapshot()
    // muss vorher von einem Nicht-Audio-Thread aufgerufen worden sein.
    void saveCurrent(File output) {
        publishSnapshot();
        auto snap = loadSnapshot();   // SAVE-OK: single load
        if (snap == nullptr) return;
        std::unique_ptr<XmlElement> xml = snap->toValueTree().createXml();
        xml->writeToFile(output, "");
    }

    void save(File output) {
        setName(output.getFileNameWithoutExtension());

        if (config != nullptr) {
            config->setName(name);
            config->setBufferSize(bufferSize);
            config->setSampleRate(sampleRate);
            config->setTracklength(tracklength);
            config->setTempo(tempo);
        }

        publishSnapshot();
        auto snap = loadSnapshot();   // SAVE-OK: single load
        if (snap == nullptr) return;
        std::unique_ptr<XmlElement> xml = snap->toValueTree().createXml();
        xml->writeToFile(output, "");
    }
    
    void load(File file) {
        std::unique_ptr<XmlElement> xml = XmlDocument(file).getDocumentElement();
        ValueTree v = ValueTree::fromXml(*xml.get());
        xml = nullptr;
        config->setConfig(v);
        setName(config->getName());
    }
    
    void clear() {
        projectAudio.clear();
    }
    
    void addAudioFile(String id, String path) {
        projectAudio.insert(make_pair(id, path));
    }
    
    String getAudioPath(String id) {
        return projectAudio[id];
    }

    static long DEFAULT_TRACK_LENGTH;
    static int  DEFAULT_TRACK_HEIGHT;
    static float DEFAULT_TEMPO;
   
    void setMidiProcessor(MidiProcessor* processor) {
        this->midiProcessor = processor;
	}

    MidiProcessor* getMidiProcessor() {
        return this->midiProcessor;
	}
    
protected:
    Project() {
        undoManager = new juce::UndoManager();
        lookAndFeel = new ElegantDarkLookAndFeel();
        this->tracklength = DEFAULT_TRACK_LENGTH;
        name = "empty Project";
        this->config = new ProjectConfig();
    }
    
    ~Project() {

        // Dialog windows are top-level windows managed by JUCE's Desktop.
        // After the main window is deleted, calling UI methods (isVisible,
        // setVisible) on them is unsafe. Just drop the references.
        openWindows.clear();

        // check if supplemental tab is still open (tab at the bottom side)

        if (supplementalTab != nullptr) {
            supplementalTab->clearTabs();
            delete supplementalTab;
        }

        // finally delete the according resources

        delete undoManager;
        delete lookAndFeel;
        delete config;
    }
    
    ApplicationCommandManager* commandManager;
    AudioDeviceManager* deviceManager;
    PropertyView* propertyView = nullptr;
    static Project* instance;
    static bool destroyed;
    juce::UndoManager* undoManager;
    juce::StringArray recentFiles;
    ElegantDarkLookAndFeel* lookAndFeel;
    MainTabbedComponent* supplementalTab = nullptr;
    std::vector<juce::DialogWindow*> openWindows;
    juce::Component* main = nullptr;
    Sampler* defaultSampler = nullptr;
    class JavaScriptEditorComponent* jsEditor = nullptr;
    Module* root = nullptr;
    AppMode mode = STUDIO;
	MidiProcessor* midiProcessor = nullptr;

    String currentFilePath;
    String currentFileName;
    bool newFile = true;
    bool dirty = false;
    
    int tolerance = 1;

    String name;
    long tracklength;
    double sampleRate = 44100;
    int bufferSize = 128;
    float tempo = DEFAULT_TEMPO;
    bool snapToGridEnabled = true;
    
    map<String, String> projectAudio;

    ProjectConfig* config = NULL;

    // ---- ProjectSnapshot (Regel B) ----
    // Single Source of Truth fuer den Save-Pfad. Erzeugt auf dem UI-Thread,
    // Audio-Thread liest hoechstens (zukuenftige Reads); Save liest genau
    // einmal load() und arbeitet ausschliesslich auf dem Snapshot.
public:
    void publishSnapshot()
    {
        auto snap = std::make_shared<synthlab::ProjectSnapshot>();
        snap->name        = name;
        snap->trackLength = tracklength;
        snap->sampleRate  = sampleRate;
        snap->bufferSize  = bufferSize;
        snap->tempo       = tempo;
        snap->snapToGrid  = snapToGridEnabled;
        if (config != nullptr)
            snap->configTree = config->getConfig();
        snap->audioFiles  = projectAudio;
        currentSnapshot.publish (snap);
    }

    synthlab::SnapshotHolder::Pointer loadSnapshot() const noexcept
    {
        return currentSnapshot.load();
    }

private:
    synthlab::SnapshotHolder currentSnapshot;
};



#endif  // PROJECT_H_INCLUDED
