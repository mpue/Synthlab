#pragma once

#include <map>
#include "../JuceLibraryCode/JuceHeader.h"
#include "SynthEditor.h"
#include "PropertyView.h"
#include "MainTabbedComponent.h"
#include "Tracks/MasterChannelPanel.h"
#include "Tracks/TrackNavigator.h"
#include "Tracks/PositionMarker.h"
#include "MixerPanel.h"
#include "ToolbarComboBox.h"
// EditorComponent ist mit dem Docking-Refactor aufgeloest worden — editor,
// mixerPanel, navigator und editorAreaTab liegen direkt in MainComponent.
#include "DefaultToolbarItemFactory.h"
#include "ModuleBrowser.h"
#include "AudioEngine/ADSR.h"
#include "MidiProcessor.h"
#include "CommandParser.h"
#include "TransportDisplayComponent.h"
#include <math.h>
#include "BounceProgressDialog.h"
#include "BounceDialog.h"
#include "Metronome.h"
#include "AudioLevelMeter.h"
#include "SpektrumAnalyzer.h"
#include "GeometryViewComponent.h"
#include "QuickstartComponent.h"
#include "JavaScriptEditorComponent.h"
#include "ControlSurfacePanel.h"
#include "UI/Docking/DockingFramework.h"

#define _USE_MATH_DEFINES
#ifndef M_PI
#define M_PI       3.14159265358979323846 
#endif

//==============================================================================

inline float fastSin(float x) {
    // x ∈ [0, π/2] ideal!
    const float B = 4 / M_PI;
    const float C = -4 / (M_PI * M_PI);
    const float P = 0.225f;

    float y = B * x + C * x * std::fabs(x);
    return P * (y * std::fabs(y) - y) + y;
}

inline float fastCos(float x) {
    return fastSin((M_PI / 2.0f) - x);
}

class BounceThread;
class ChatGPTInterface;
class StatusBarComponent;
class ShellConsoleComponent;
class Track;
class PianoRollEditor;
class MidiRegionEventEditor;

class MainComponent   : public juce::AudioAppComponent,
                        public juce::MenuBarModel,
                        public juce::KeyListener,
                        public juce::Button::Listener,
                        public juce::Timer,                        
                        public ApplicationCommandTarget,
                        public juce::ChangeListener,
                        public EditorListener,
                        public ComboBox::Listener
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent();

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;
    
    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;
    bool keyStateChanged (bool isKeyDown, juce::Component* originatingComponent) override;
    bool keyPressed (const juce::KeyPress& key, juce::Component* originatingComponent) override;
    void modifierKeysChanged (const juce::ModifierKeys& modifiers) override;
    void createKeyMap();
    void buttonClicked (juce::Button*) override;
    virtual void fileChanged(String name) override;
    virtual void selectionChanged(juce::Component* m) override;
    virtual void dirtyChanged(bool dirty) override;
    virtual void comboBoxChanged (ComboBox* comboBoxThatHasChanged) override;
    
    void processModule(Module* m);
    int getNumActiveChannels(int i);
    void timerCallback() override;
    virtual void changeListenerCallback (ChangeBroadcaster* source) override;
    void startBounce(const juce::File& file, double duration, bool exportMp3 = false, bool exportOgg = false);
    void showBounceDialog();
    juce::String tryExecuteChatInstruction(const std::string& jsonString);

    // Persistierter Claude API Key — gespeichert in ~/.Synthlab/claude_api_key.txt.
    // Plain text, dasselbe Verzeichnis-Schema wie der bestehende Plugin-/Audio-Config.
    juce::String loadClaudeApiKey() const;
    void saveClaudeApiKey(const juce::String& key);
    void showClaudeApiKeyDialog();
    void showDeactivateLicenseDialog();
    void showLicenseServerDialog();
    void showAutoArrangeDialog();

    // command handling
    virtual ApplicationCommandTarget* getNextCommandTarget() override;
    virtual void getAllCommands(Array<CommandID>& commands) override;
    void getCommandInfo(CommandID commandID, ApplicationCommandInfo& result) override;
    virtual bool perform(const InvocationInfo& info) override;

    enum CommandIds {
        undoCommand = 111,
		redoCommand = 112 
    };


    SynthEditor* getEditor() {
        return editor;
    }

    // default settings just in case
    double sampleRate = 44100;
    int buffersize = 128;

    void setRunning(bool b) {

        if (b)
            DBG("Set running to true");
        else
            DBG("Set running to false");

        running.store(b);
    }

    bool isRunning() {
        return running.load();
    }

    void setBouncing(bool b) {
        bouncing.store(b);
    }

    bool isBouncing() {
        return bouncing.load();
    }


    // shared_ptr, weil TrackNavigator frueher in EditorComponent als
    // shared_ptr lebte (Bestands-Konvention) und SynthEditor::setNavigator
    // den raw Pointer per .get() weiterreicht.
    std::shared_ptr<TrackNavigator> navigator = nullptr;
    MidiProcessor* midiProcessor = nullptr;

    // Piano-Roll als eigenes Dock-Panel. Aufgerufen vom TrackNavigator bei
    // Doppelklick auf eine MidiRegion. Editor wird hier vom Aufrufer mit
    // verkabelten Callbacks geliefert; Ownership wandert ins DockablePanel.
    void showOrCreatePianoRollPanel (std::unique_ptr<PianoRollEditor> editor);
    void closePianoRollPanel();

    // MIDI-Event-Editor analog: eigenes Dock-Panel, vom TrackNavigator bei
    // Cmd+M (Region-Menu "Event editor") gefuellt. Owner ist das Panel.
    void showOrCreateEventEditorPanel (std::unique_ptr<MidiRegionEventEditor> editor);
    void closeEventEditorPanel();

private:
    //==============================================================================

    std::atomic<bool> bouncing{ false };
    std::atomic<bool> running{ false };
    void openSettings();
    void openQuickstart();
    void openJavaScriptEditor();
    // Erzeugt + verkabelt eine neue JavaScriptEditorComponent (setzt jsEditor
    // und den Project-Pointer). Owner ist der Aufrufer bis zum createPanel().
    JavaScriptEditorComponent* createJavaScriptEditorComponent();
    // Bringt das JS-Panel in den Vordergrund oder legt es neu an, falls es
    // geschlossen wurde. Vom Menue ("JavaScript Editor") genutzt.
    void showOrCreateJavaScriptPanel();
    // SynthEditor::cleanUp() (am Anfang von openFile aufgerufen) entfernt alle
    // Tabs ausser Editor. Nach dem Laden bringt diese Helper den Navigator-Tab
    // wieder zurueck und in den Vordergrund — ueber alle File-Open-Pfade
    // (Toolbar, Recent-Menu, Quickstart) hinweg dieselbe Logik.
    void restoreNavigatorAfterOpen();

    // Delete-Shortcut und Toolbar-Button landen hier oben, weil der Sub-Editor
    // den Key nicht selbst konsumiert (siehe SynthEditor::keyPressed). Wenn ein
    // Sub-Modul-Tab aktiv ist, muss der Aufruf gegen genau diesen Editor laufen
    // — der Root-`editor` hat dann eine leere Selection und es passiert nichts.
    SynthEditor* getActiveSubEditor() const;

	// editor: Raw-Pointer-Member (existierende Callsites bleiben). Lifetime
	// haengt am editorOwned unique_ptr, deklariert nach dockManager — damit
	// editorOwned NACH dockManager zerstoert wird (Member-Reverse-Order):
	// dockManager raumt die Editor-DockablePanel ab (die nur den
	// editorViewport als Content haelt), dann faellt editorOwned, dann
	// SynthEditor::~ runs gegen ein bereits sauberes Project-State.
	SynthEditor* editor = nullptr;
    std::unique_ptr<MainTabbedComponent> tab = nullptr;
    // Ownership der drei dockbaren Hauptpanels liegt jetzt beim DockManager
    // (jedes Panel wird via dock.createPanel mit std::unique_ptr<Component>
    // uebergeben). MainComponent haelt rohe Pointer fuer den Zugriff — solange
    // der DockManager existiert, ist der Pointer gueltig.
    PropertyView* propertyView = nullptr;
    std::unique_ptr<juce::MenuBarComponent> menu = nullptr;
    std::unique_ptr<ModuleBrowser> moduleBrowser = nullptr;
    ToolbarComboBox* layerCombobox = nullptr;

	std::shared_ptr<ApplicationCommandManager> commandManager = nullptr;

    juce::Label* cpuLoadLabel = nullptr;
    juce::Slider* loadSlider = nullptr;

    std::shared_ptr<MixerPanel> mixerPanel = nullptr;
    juce::PopupMenu* pluginMenu = nullptr;

    // Editor-Bereich liegt jetzt direkt in MainComponent (EditorComponent
    // wurde aufgeloest, siehe feat/docking Phase 2). DockablePanel "Editor"
    // bekommt editorAreaTab als Content. Lifetime der inneren Components:
    // editorAreaTab gehoert dem Panel (per unique_ptr beim createPanel
    // uebergeben), editorViewport/trackViewport leben als unique_ptr in
    // MainComponent, die SynthEditor- und TrackNavigator-Komponenten haengen
    // viewed-by-Viewport (mit deleteWhenRemoved=false; MainComponent ownt
    // editor via unique_ptr<SynthEditor>, navigator als shared_ptr).
    // Raw-Member: Ownership liegt beim "Editor" DockablePanel. Gueltig
    // solange dockManager existiert (Editor-Panel ist closable=false).
    MainTabbedComponent* editorAreaTab = nullptr;
    std::unique_ptr<juce::Viewport>      editorViewport = nullptr;
    std::unique_ptr<juce::Viewport>      trackViewport  = nullptr;
    std::unique_ptr<ShellConsoleComponent> consoleComponent = nullptr;
    ImageButton* lockButton = nullptr;
    CommandParser* parser = nullptr;

    // Methoden, die frueher EditorComponent zur Verfuegung stellte.
    void openTrackView();
    void importAudio();
    void importMidi();
    void importMidiFileToTrack(const juce::MidiFile& midiFile, Track* track);

    // Show-X / Re-Open-Helfer fuer das View-Menue: aktiviert das Panel,
    // wenn vorhanden; sonst Re-Create.
    void showOrCreateLogPanel();
    void showOrCreateLevelMeterPanel();
    void showOrCreateSpectrumPanel();

    // Verdrahtet die statischen PluginModule-Hooks mit dem DockManager, damit
    // Plugin-Editoren optional als Dock-Panels statt Floating-Fenster erscheinen.
    void installPluginDockHooks();

    // Browser/Instruments/Description: schliessbar, ueber das View-Menue
    // wiederherstellbar. Content lebt in PropertyView, ueberlebt das Close.
    void showOrCreateBrowserPanel();
    void showOrCreateInstrumentsPanel();
    void showOrCreateDescriptionPanel();
    void showOrCreateNavigatorPanel();

    virtual juce::StringArray getMenuBarNames() override;
    virtual juce::PopupMenu getMenuForIndex(int index, const juce::String & menuName) override;
    virtual void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;
    
    std::unique_ptr<juce::XmlElement> loadAudioConfig();

    // Auto-Save: vom 40-ms-Timer hochgezaehlt; bei Erreichen des in den
    // Settings konfigurierten Intervalls wird ein Recovery-Snapshot
    // geschrieben, sofern das Projekt ungespeicherte Aenderungen hat.
    int  autoSaveTickCounter = 0;
    bool autoSaveInProgress  = false;
    void serviceAutoSave();

    // Beim Start: liegt ein Recovery-Snapshot einer nicht sauber beendeten
    // Vorsession vor, einen Wiederherstellungs-Dialog anbieten. Wird nach dem
    // Aufbau des Hauptfensters per callAfterDelay angestossen.
    void maybeOfferCrashRecovery();

    void createConfig();
    void createToolbar();
    void createMenu();
    void createCPUMeter();
    void createStudioLayout();
    void createPlayerLayout();
    
    // Studio-Layout laeuft jetzt ueber den docking::DockManager. Default-
    // Anordnung wird in createStudioLayout aufgebaut, manuelle Aenderungen
    // werden ueber saveLayoutFile()/loadLayoutFile() persistiert (.synthlabdock).
    std::unique_ptr<docking::DockManager> dockManager;

    // Editor-Lifetime-Owner: nach dockManager deklariert, sodass die Member-
    // Reverse-Destruktion erst die DockablePanels abraumt (incl. editorAreaTab
    // im Editor-Panel, das nur den editorViewport als Content haelt) und dann
    // editorOwned ~SynthEditor laufen laesst. Project-Pointer auf PropertyView
    // sind zu dem Zeitpunkt schon genullt (siehe destructor unten).
    std::unique_ptr<SynthEditor> editorOwned = nullptr;
    docking::DockablePanel* propertiesDockPanel = nullptr;
    docking::DockablePanel* editorDockPanel     = nullptr;
    docking::DockablePanel* jsDockPanel         = nullptr;
    // Browser/Instruments/Description: eigenstaendige Panels neben Properties.
    // Content ist ein WeakChildHolder; die echten Komponenten besitzt
    // PropertyView (getBrowser/getInstrumentBrowser/getDescriptionEditor).
    docking::DockablePanel* browserDockPanel      = nullptr;
    docking::DockablePanel* instrumentsDockPanel  = nullptr;
    docking::DockablePanel* descriptionDockPanel  = nullptr;
    // Navigator + Quickstart: aus dem editorAreaTab herausgeloest, jetzt
    // eigenstaendige (schliessbare) DockablePanels. Content ist ein
    // WeakChildHolder; die echten Komponenten besitzt MainComponent weiter
    // (trackViewport bzw. quickstart), sodass ein Close die Teardown-
    // Reihenfolge im Dtor nicht beruehrt.
    docking::DockablePanel* navigatorDockPanel  = nullptr;
    docking::DockablePanel* quickstartDockPanel = nullptr;
    // Bottom-Row Panels: einzeln dock-/floatbar. Mixer + Tools sind nicht
    // schliessbar (komplexe Lifetimes), Log/Level/Spektrum sind closable —
    // ihre Audio-Hooks lesen ueber Atomic-Refs (siehe levelMeterAudio /
    // spectrumAudio), sodass das Schliessen sicher ist.
    docking::DockablePanel* mixerDockPanel    = nullptr;
    docking::DockablePanel* toolsDockPanel    = nullptr;
    docking::DockablePanel* logDockPanel      = nullptr;
    docking::DockablePanel* levelDockPanel    = nullptr;
    docking::DockablePanel* spectrumDockPanel = nullptr;
    // Raw-Pointer auf den MainTabbedComponent, der jetzt Project::getSupplemental
    // beliefert. Lebt im ToolsDockContent (DockablePanel-Content) — gueltig
    // solange das Tools-Panel im DockManager existiert.
    MainTabbedComponent* toolsTab = nullptr;
    
    std::map<int,int> keyCodeMidiNote;
    
    int currentOctave = 0;

    std::unique_ptr<DefaultToolbarItemFactory> toolbarFactory;
    std::unique_ptr<juce::Toolbar> toolbar = nullptr;
    // Non-owning — owned by the Toolbar via getItemComponent().
    class TransportToolbarButton* playToolbarButton = nullptr;
    class TransportToolbarButton* recordToolbarButton = nullptr;
    
    bool initialized = false;   
    bool keyStates[128] = { false };
    
    float magnitudeLeft = 0;
    float magnitudeRight = 0;
    
    float magnitudeLeftIn = 0;
    float magnitudeRightIn = 0;
    
    // Zeitmessung der Audio-Last in Millisekunden mit Nachkommastellen.
    // Ein Audio-Block dauert i.d.R. << 1 ms, daher MUSS hier double stehen —
    // mit long wurde die Dauer auf 0/1 ms gerundet und die CPU-Anzeige sprang.
    double currentTime = 0.0;
    double lastTime = 0.0;

    int layer = 0;
    double cpuLoad = 0.0;
    double loads[10] = {0.0};
    int currentMeasure = 0;
    int currentSample = 0;
    
    StringArray sampleData;
    
    bool isOptionPressed = false;
    
    Sampler* defaultSampler = nullptr;

    std::unique_ptr<ChatGPTInterface> chat = nullptr;    
	std::unique_ptr<TransportDisplayComponent> transportDisplay = nullptr;
    std::unique_ptr<BounceThread> bounceThread;
    std::atomic<double> bounceProgress{ 0.0 };
    std::unique_ptr<Metronome> metronome;
    juce::AudioBuffer<float> metronomeBuffer;
    juce::AudioBuffer<float> inputCaptureBuffer; // Snapshot of hardware input before output clear
    
	std::unique_ptr<AudioLevelMeter> levelMeter = nullptr;
    std::unique_ptr<SpectrumAnalyzer> spectrumAnalyzer = nullptr;

    // Audio-Thread liest diese atomar — UI-Thread setzt sie bei Construction
    // der zugehoerigen Komponente und nullt sie bei Destruction. Damit
    // koennen LevelMeter/SpectrumAnalyzer spaeter in schliessbaren
    // DockablePanels leben, ohne dass getNextAudioBlock auf einen freed
    // pointer dereft. Solange noch beide Felder (unique_ptr + atomic)
    // existieren, hat das Atomic den gleichen Wert wie der unique_ptr.
    std::atomic<AudioLevelMeter*>  levelMeterAudio { nullptr };
    std::atomic<SpectrumAnalyzer*> spectrumAudio   { nullptr };
    std::unique_ptr<QuickstartComponent> quickstart = nullptr;
    // jsEditor wird vom DockManager (via DockablePanel) besitzt. Raw-Pointer
    // bleibt fuer die bestehenden setX-Verdrahtungen (Project::setJS, etc.) gueltig.
    JavaScriptEditorComponent* jsEditor = nullptr;
    std::unique_ptr<ControlSurfacePanel> controlSurfacePanel = nullptr;

    // Persistenz-Hilfen fuer Dock-Layout.
    juce::File getDockLayoutFile() const;
    void saveDockLayout();
    void loadDockLayout();
    void resetDockLayout();

    // Filled by getMenuForIndex when the Macros menu is built — indices into
    // this vector are the menu item IDs minus kMacroMenuIdBase. menuItemSelected
    // uses it to look up which .js file to run for a clicked menu entry.
    std::vector<juce::File> macroFiles;

    // Same pattern for the Tests menu — but rooted at ~/.Synthlab/test/ and
    // with its own ID base so the two ranges don't collide.
    std::vector<juce::File> testFiles;

    // Add-Module-Track Picker: scannt ~/.Synthlab/modules/ rekursiv nach .slb-
    // Dateien mit isInstrument=1. moduleTrackFiles[idx] gehört zur Menu-ID
    // (idx + kModuleTrackMenuIdBase). Liefert true, wenn mindestens ein Modul
    // gefunden wurde.
    std::vector<juce::File> moduleTrackFiles;
    bool buildAddModuleTrackMenu(juce::PopupMenu& menu);

    // Statuszeile am unteren Fensterrand (Studio-Modus). CPU/RAM/Audio/Tempo.
    std::unique_ptr<StatusBarComponent> statusBar = nullptr;

	std::unique_ptr<GeometryViewComponent> rotatingCube = nullptr;

    CriticalSection lock;

    // App-weites Tooltip-Fenster. Ohne eine TooltipWindow-Instanz zeigt JUCE
    // gar keine Tooltips an (auch nicht die per setTooltip gesetzten). An
    // `this` gehaengt, damit es den gesamten Editor abdeckt.
    juce::TooltipWindow tooltipWindow { this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
