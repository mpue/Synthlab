#pragma once

//[Headers]     -- You can add your own extra header files here --
#include "../JuceLibraryCode/JuceHeader.h"
#include "MultiComponentDragger.h"
#include "Module.h"
#include "Connection.h"
#include "AudioIn.h"
#include "AudioOut.h"
#include "AuxOut.h"
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

class SamplerModule;

class SynthEditor  : public Component,
                     public ChangeBroadcaster,
                     public AudioModule,
                     public Timer,
                     public ChangeListener,
                     public DragAndDropTarget,
                     public IEditor,
                     public ApplicationCommandTarget
{
public:

    SynthEditor (double sampleRate, int buffersize );
    SynthEditor ();
    ~SynthEditor();

    virtual void changeListenerCallback (ChangeBroadcaster* source) override;
    
    // load and save
    
    void cleanUp();
    void newFile();
    void saveFile();
    void openFile();
    void openTracks(File file);
    void openSampleEditor(SamplerModule* sm);
    void openStepSequencer(StepSequencerModule* ssm);
    void openEditor(Module* m);
    void openRecorder(AudioRecorderModule* arm);
    void loadFromString(String in);
    Module* loadModule();
    void saveModule(Module* m);
    void configureAudioModule(Module *m, ChangeBroadcaster* broadcaster);
    Track* createTrack();
    // audio processing

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate);
    void showContextMenu(Point<int> position);

    bool isRunning();
    void setRunning(bool running);
    void timerCallback() override;

    // editing

    int addChannel(String name, Mixer::Channel::Type channeltype);
    void addConnection(const MouseEvent& e, Module* m);
    bool connectionExists(std::vector<Connection*> connections, Connection* c);
    SelectionModel& getSelectionModel();
    void setCurrentLayer(int layer);
    int getCurrentLayer();
    void duplicateSelected();
    void setLocked(bool lock) {
        this->locked = lock;
        setModuleLocked(this->getModule(), lock);
        selectionModel.clearSelection();
        repaint();
    }
    
    bool isLocked() {
        return this->locked;
    }
    
    void setModuleLocked(Module* m, bool locked) {
        
        if (m != nullptr) {
            
            m->setLocked(locked);
            
            for (int i = 0; i< m->getModules()->size();i++) {
                if (m->getModules()->at(i) != nullptr) {
                    m->getModules()->at(i)->setLocked(locked);
                }
            }
        }
    }
    
    void drawConnections(Graphics& g);
    
    int snap(int location, int raster);
    void dragSelection();
    void moveSelection(const MouseEvent& e);
    void checkForConnectionDrag();
    
    void addControl(Module* module, int pinIndex);
    void addConstant(Module* module, int pinIndex);
    void addTerminal(Module* module, int pinIndex);
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
    bool keyPressed (const KeyPress& key) override;
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
        EXPORT_AUDIO = 13
    };
    
    // member access
    
    int getIndex();
    void setIndex(int index);
    
    void setTab(TabbedComponent* t);
    void setModule(Module * m, bool deleteWhenRemoved);
    Module* getModule();
    void setMixer(MixerPanel* mixer);
    MixerPanel* getMixer();

    bool isDirty() override;
    void setDirty(bool dirty) override;
                         
    void setSamplerate(double rate);
    virtual float getSampleRate() override;
    
    void setBufferSize(int buffersize);
    virtual float getBufferSize() override;
    
    void addEditorListener(EditorListener* listener);
    void saveScreenShot();
    
    void alignSelectedY();
    void alignSelectedX();
    
    void resetGUIPosition();
    void updateProject(File file);
    void updateProject(URL url);

    void setNavigator(TrackNavigator* navigator);

    void exportAudio();

private:
    //[UserVariables]   -- You can add your own custom variables in this section.

    void notifyListeners();
    void notifyDirtyListeners();
    
    bool snapToGrid = true;
                         
    bool isAltDown = false;
    bool isCtrlDown = false;
    bool isLeftShiftDown = false;
    bool isLeftMouseDown = false;
	bool isMovingModule = true;
	bool isDraggingConnection = false;

    Module* root = nullptr;
    
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

    void drawCurrentConnection();
    
    SelectionModel::State state = SelectionModel::NONE;
    TabbedComponent* tab = nullptr;
    Rectangle<int> selectionFrame = Rectangle<int>(0, 0, 0, 0);
    
    static String defaultMidiInputName;
    static String defaultMidiOutputName;
    
    bool dirty = false;
                         
    int currentSample = 0;
    int bufferSize;
    double _sampleRate;
    bool running = false;
    bool locked = false;
    bool dragHasStarted = false;
    Module::Layer currentLayer = Module::Layer::ALL;
    
    SelectionModel selectionModel;
  
    MixerPanel* mixer = nullptr;
    
    bool deleteModuleWhenRemoved = true;
    
    int index = 0;
    
    Path linePath;
    Point<int> p1;
    Point<int> p2;
    
    std::vector<Viewport*> openViews;
    std::vector<EditorListener*> listener;
    
    TrackNavigator* navigator = nullptr;

    //[/UserVariables]

    
    //==============================================================================


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthEditor)
};

//[EndFile] You can add extra defines here...
//[/EndFile]
