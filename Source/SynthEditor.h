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
#include "PrefabFactory.h"
#include <vector>
#include "MixerPanel.h"
#include "Project.h"

class SamplerModule;

class SynthEditor  : public Component,
                     public ChangeBroadcaster,
                     public AudioModule,
                     public Timer,
                     public DragAndDropTarget {
public:

    SynthEditor (double sampleRate, int buffersize );
    SynthEditor ();
    ~SynthEditor();

    // load and save
    
    void cleanUp();
    void newFile();
    void saveFile();
    void openFile();
    void openSampleEditor(SamplerModule* sm);
    void openStepSequencer(StepSequencerModule* ssm);
    void openEditor(Module* m);
    void loadFromString(String in);
    void saveStructure(std::vector<Module*>* modules, std::vector<Connection*>* connections, ValueTree* v);
    void loadStructure(std::vector<Module*>* modules, std::vector<Connection*>* connections,ValueTree* v);

    // audio processing

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate);
    void showContextMenu(Point<int> position);
    bool channelIsValid(int channel);
    bool auxChannelIsValid(int channel, int subchannel);
    bool isRunning();
    void setRunning(bool running);
    void timerCallback() override;

    // editing

    int addChannel(String name, Mixer::Channel::Type channeltype);
    void addConnection(const MouseEvent& e, Module* m);
    void deleteSelected(bool deleteAll);
    bool connectionExists(std::vector<Connection*> connections, Connection* c);
    void removeModule(Module* module);
    SelectionModel& getSelectionModel();
    void removeSelectedItem();
    void setCurrentLayer(int layer);
    void duplicateSelected();
    
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

    // member access
    
    void setTab(TabbedComponent* t);
    void setModule(Module * m, bool deleteWhenRemoved);
    Module* getModule();
    void setMixer(MixerPanel* mixer);
    MixerPanel* getMixer();

    std::vector<AudioIn*>& getInputChannels();
    std::vector<AudioOut*>& getOutputChannels();
    std::vector<AuxOut*>& getAuxChannels();
    
    void setSamplerate(double rate);
    virtual float getSampleRate() override;
    
    void setBufferSize(int buffersize);
    virtual float getBufferSize() override;
    
private:
    //[UserVariables]   -- You can add your own custom variables in this section.

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

    std::vector<AudioOut*> outputChannels;
    std::vector<AudioIn*> inputChannels;
    std::vector<AuxOut*> auxChannels;
    
    static String defaultMidiInputName;
    static String defaultMidiOutputName;
    
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
    
    Path linePath;
    Point<int> p1;
    Point<int> p2;
    
    std::vector<Viewport*> openViews;
                         
    //[/UserVariables]

    
    //==============================================================================


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthEditor)
};

//[EndFile] You can add extra defines here...
//[/EndFile]
