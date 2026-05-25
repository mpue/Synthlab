#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "MultiComponentDragger.h"
#include "Module.h"
#include "Connection.h"
#include "AudioIn.h"
#include "AudioOut.h"
#include "AuxOut.h"
#include "BusOut.h"
#include "AudioModule.h"
#include "SampleEditor.h"
#include "SelectionModel.h"
#include "StepSequencerModule.h"
#include "AudioRecorderModule.h"
#include "PrefabFactory.h"
#include <vector>
#include "MixerPanel.h"
#include "Mixer.h"
#include "Project/Project.h"
#include "EditorListener.h"
#include "IEditor.h"
#include "Tracks/TrackNavigator.h"
#include "UITools.h"

class SamplerModule;

class SynthEditor  : public Component,
                     public ChangeBroadcaster,
                     public AudioModule,
                     public Timer,
                     public ChangeListener,
                     public DragAndDropTarget,
                     public IEditor,
                     public ApplicationCommandTarget,
                     public KeyListener
                
{
public:

    SynthEditor (double sampleRate, int buffersize );
    SynthEditor ();
    ~SynthEditor();

    virtual void changeListenerCallback (ChangeBroadcaster* source) override;
    
    // load and save
    
    void cleanUp();
    // Reset to an empty project. If askIfDirty is true (default) and there
    // are unsaved changes, prompt the user first; on cancel, leave state
    // untouched and return without tearing anything down. Load paths pass
    // false to skip the prompt (the file picker is the confirmation).
    void newFile(bool askIfDirty = true);

    // Returns true if it's safe to discard the current project state --
    // either the project is not dirty, or the user confirmed the dialog.
    // All paths that follow a `cleanUp(); newFile(); ...` sequence must
    // call this FIRST and bail out on false. Asking the dialog after the
    // teardown is the bug that produced the New > Cancel crash.
    bool confirmDiscardIfDirty();
    void saveFile();
    void saveFileAs();
    void openFile();
    // Load a project directly from a file (no chooser). Both the chooser-based
    // openFile() and external callers (main menu "Open Recent") route through
    // this so every load path runs the same module+track restore sequence.
    void openFile(juce::File file);

    // Save the current project to `target` without opening a chooser dialog.
    // Used by synthlab.project.save() — flips Project's currentFilePath +
    // newFile state to redirect saveFile()'s write to the target, then runs
    // the normal save pipeline. Returns true if the file exists afterwards.
    // NOTE: this DOES mutate Project::currentFilePath as a side effect; for
    // the scripted-test use case that's acceptable, the test cleans up.
    bool saveFileTo(const juce::File& target);

    // Load a project from `source` without showing the "discard dirty?"
    // prompt. The dirty flag is cleared first so confirmDiscardIfDirty()
    // returns true silently. Returns false if the file doesn't exist.
    bool openFileNoPrompt(const juce::File& source);
    void openTracks(File file);
    void loadMidiRegions(juce::ValueTree& track, Track* t);
    void openSampleEditor(SamplerModule* sm);
    void openStepSequencer(StepSequencerModule* ssm);
    void openEditor(Module* m);
    void openRecorder(AudioRecorderModule* arm);
    void loadFromString(String in);
    Module* loadModule();
    void saveModule(Module* m);
    void configureAudioModule(Module *m, ChangeBroadcaster* broadcaster);
    void addVirtualInstrumentTrack();
    // Overload that skips the plugin browser dialog and creates a track for a
    // pre-known plugin name — used by the Instruments tab in PropertyView.
    void addVirtualInstrumentTrack(juce::String pluginName);

    // Smart layout helpers
    juce::Point<int> findFreePosition(juce::Point<int> desired, int w, int h) const;
    juce::Point<int> getNextTrackStartPosition() const;

    // Deletes the current selection, but cascades any selected TrackIn module
    // into a full TrackDeleteAction (removes the Track plus its TrackIn,
    // AudioIn/MidiGate inputs, AudioOut/AuxOut/MidiOut outputs, PluginModule
    // for instrument tracks, and every connection that wired them). Whatever
    // is left in the selection after the TrackIn cascade is handed to a normal
    // RemoveSelectedAction. All inside a single undo transaction.
    static void deleteSelectionWithCascade(SynthEditor* editor,
                                           TrackNavigator* navigator);
    
    // audio processing

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate);

    void showCommonContextMenu(Point<int> position);
    bool showPinContextMenu(Point<int> position, Module* module, int pinIndex);
    bool showModuleContextMenu(Point<int> position, Module* module);
    void showPrefabMenu(Point<int> position);
    bool isRunning();
    void setRunning(bool running);
    void timerCallback() override;

    // editing

    int addChannel(String name, Mixer::Channel::Type channeltype);
    void addConnection(const MouseEvent& e, Module* m);
    SelectionModel& getSelectionModel();
    void setCurrentLayer(int layer);
    int getCurrentLayer();
	Module::Layer getCurrentLayerEnum();
    void duplicateSelected();
    void setLocked(bool lock);
    bool isLocked();
    void setModuleLocked(Module* m, bool locked);    
    void drawConnections(Graphics& g);    
    void dragSelection();
    void moveSelection(const MouseEvent& e);
    void checkForConnectionDrag();
    
    void addControl(Module* module, int pinIndex);
    void addConstant(Module* module, int pinIndex);
    void addTerminal(Module* module, int pinIndex);
    void createModuleFromSelection();
    void connect(Module* root, Module* c, Module* module , int pinIndex, int mouseY);
    
    // drag and drop
    
    void itemDropped (const SourceDetails& dragSourceDetails) override;
    bool isInterestedInDragSource (const SourceDetails& dragSourceDetails) override;
    
    // component methods
    
    void paint (Graphics& g) override;
    void resized() override;
    void mouseMove (const MouseEvent& e) override;
    void mouseDown (const MouseEvent& e) override;
    void mouseDrag (const MouseEvent& e) override;
    void mouseUp (const MouseEvent& e) override;
    void mouseDoubleClick (const MouseEvent& e) override;
    void mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& wheel) override;

    // Editor canvas zoom (Cmd/Ctrl + mouse wheel).
    void setEditorZoom(float newZoom, juce::Point<int> anchorInLocalCoords);
    float getEditorZoom() const { return editorZoom; }
    bool keyPressed (const KeyPress& key, Component* originatingComponent) override;
    bool keyStateChanged (bool isKeyDown) override;
    void modifierKeysChanged (const ModifierKeys& modifiers) override;

    // command handling
    virtual ApplicationCommandTarget* getNextCommandTarget() override;
    virtual void getAllCommands (Array<CommandID>& commands) override;
    void getCommandInfo (CommandID commandID, ApplicationCommandInfo& result) override;
    virtual bool perform (const InvocationInfo& info) override;
    
    enum CommandIds {
        ADD_MODULE = 1,
        SAVE = 2,
        LOAD = 3,
        NEW = 4,
        LOAD_MODULE = 5,
        SAVE_MODULE = 6,
        SAVE_SCREENSHOT = 7,
        DUPLICATE = 8,
        DELETE_SELECTED = 9,
        ALIGN_Y = 10,
        ALIGN_X = 11,
        RESET_GUI_POS = 12,
        EXPORT_AUDIO = 13,
        ALIGN_TO_GRID = 14,
        SAVE_AS = 15
    };

    // member access
    
    int getIndex();
    void setIndex(int index);    
    void setTab(TabbedComponent* t);
    void setModule(std::unique_ptr<Module> m, bool deleteWhenRemoved);
    void setModuleRef(Module* m);
    Module* getModule();
    void setMixer(std::shared_ptr<MixerPanel> mixer);
    std::shared_ptr<MixerPanel> getMixer();

    bool isDirty() override;
    void setDirty(bool dirty) override;
                         
    void setSamplerate(double rate);
    virtual float getSampleRate() override;
    
    void setBufferSize(int buffersize);
    virtual float getBufferSize() override;
    
    void addEditorListener(EditorListener* listener);
    void removeEditorListener(EditorListener* listener);
    void saveScreenShot();
    
    void alignSelectedY();
    void alignSelectedX();    
    void alignToGrid();
    void resetGUIPosition();

    void updateProject(File file);
    void updateProject(URL url);

    void setNavigator(TrackNavigator* navigator);

    void exportAudio();

    void cutConnectionAtPosition(Point<int> position);
    void cutConnectionsAlongLine(Point<int> start, Point<int> end);
    void removeConnection(Connection* c);


private:
    //[UserVariables]   -- You can add your own custom variables in this section.

    void notifyListeners();
    void notifyDirtyListeners();
    void drawCurrentConnection();
    
    int currentSample = 0;
    int bufferSize;
    double _sampleRate;
    bool running = false;
    bool locked = false;
    bool dragHasStarted = false;
    bool snapToGrid = false;
                             
    bool isAltDown = false;
    bool isCtrlDown = false;
    bool isLeftShiftDown = false;
    bool isLeftMouseDown = false;
	bool isMovingModule = true;
	bool isDraggingConnection = false;

    std::shared_ptr <Module> root = nullptr;
    
    bool isRoot = true;;

	int mouseX = 0;
	int mouseY = 0;

    int lineStartX = 0;
    int lineStartY = 0;
    int lineStopX = 0;
    int lineStopY = 0;

	int dragStartX = 0;
	int dragStartY = 0;
    
    int mouseDownX = 0;
    int mouseDownY = 0;

    int dragDistanceX = 0;
    int dragDistanceY = 0;
    
    
	int maxrepaintw = 0;
	int maxrepainth = 0;
	int minrepaintx = 0;
	int minrepainty = 0;

    Pin* startPin = nullptr;    
    
    SelectionModel::State state = SelectionModel::NONE;
    TabbedComponent* tab = nullptr;
    Rectangle<int> selectionFrame = Rectangle<int>(0, 0, 0, 0);
    
    static String defaultMidiInputName;
    static String defaultMidiOutputName;
    
    bool dirty = false;
                         
    Module::Layer currentLayer = Module::Layer::ALL;    
    SelectionModel selectionModel;
  
    std::shared_ptr<MixerPanel> mixer = nullptr;
    
    bool deleteModuleWhenRemoved = true;
    
    int index = 0;
    
    Path linePath;
    Point<int> p1;
    Point<int> p2;
    
    std::vector<Viewport*> openViews;
    std::vector<EditorListener*> listener;
    
    TrackNavigator* navigator = nullptr;

    bool isMiddleMouseDown = false;

    float editorZoom = 1.0f;
    static constexpr float kMinZoom = 0.25f;
    static constexpr float kMaxZoom = 4.0f;

    struct RootLevelProxy {
        Module* module = nullptr;
        juce::Component* originalParent = nullptr;
        juce::Point<int> originalPosition;
        Module::Layer originalLayer = Module::Layer::ALL;
    };

    std::vector<RootLevelProxy> rootLevelProxies;
    juce::OwnedArray<juce::Component> rootLevelGroups;

    void collectRootLevelControls(Module* container);
    void restoreRootLevelControls();
    void updateRootLevelGroupEditMode();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthEditor)
};

