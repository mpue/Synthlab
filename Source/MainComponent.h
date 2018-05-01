/*
  ==============================================================================


  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "SynthEditor.h"
#include "PropertyView.h"
#include "MainTabbedComponent.h"
#include "MasterChannelPanel.h"
#include "MixerPanel.h"
#include "ToolbarComboBox.h"
#include "EditorComponent.h"
#include "DefaultToolbarItemFactory.h"
#include "ModuleBrowser.h"
#include <map>
//==============================================================================

class MainComponent   : public AudioAppComponent,
                        public MenuBarModel,
                        public KeyListener,
                        public Button::Listener,
                        public Timer,
                        public AudioProcessorPlayer,
                        public DragAndDropContainer
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent();

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;
    
    //==============================================================================
    void paint (Graphics& g) override;
    void resized() override;
    bool keyStateChanged (bool isKeyDown, Component* originatingComponent) override;
    bool keyPressed (const KeyPress& key, Component* originatingComponent) override;
    void modifierKeysChanged (const ModifierKeys& modifiers) override;
    void createKeyMap();
    void buttonClicked (Button*) override;
    
    
    void handleIncomingMidiMessage (MidiInput* source, const MidiMessage& message) override;
    void sendGateMessage(Module* module, int channel, int velocity,int note,bool on);
    void sendNoteMessage(Module* module, int channel, int note);
    void sendControllerMessage(Module* module, int channel, int controller, float value);
    void processModule(Module* m);
    int getNumActiveChannels(int i);
    
    SynthEditor* getEditor() {
        return editor;
    }
    
    void mouseDrag (const MouseEvent& event) override;
    virtual void dragOperationStarted (const DragAndDropTarget::SourceDetails& details) override;
    void timerCallback() override;
    
private:
    //==============================================================================

    void openSettings();
    
	SynthEditor* editor = nullptr;
    MainTabbedComponent* tab = nullptr;
    PropertyView* propertyView = nullptr    ;
    Viewport* view = nullptr;
    MenuBarComponent* menu;
    ModuleBrowser* moduleBrowser = nullptr;

    Label* cpuLoadLabel = nullptr;
    Slider* loadSlider = nullptr;

    MixerPanel* mixerPanel = nullptr;
    Mixer* mixer = nullptr;
    PopupMenu* pluginMenu = nullptr;
    
    EditorComponent* editorView = nullptr;
    
    virtual StringArray getMenuBarNames() override;
    virtual PopupMenu getMenuForIndex(int index, const String & menuName) override;
    virtual void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;
    
    StretchableLayoutManager stretchableManager;
    StretchableLayoutResizerBar resizerBar;
    
    std::map<int,int> keyCodeMidiNote;
    
    int currentOctave = 1;
    DefaultToolbarItemFactory* toolbarFactory;
    Toolbar* toolbar = nullptr;
    
    bool initialized = false;
    volatile bool running = false;
    
    // default settings just in case
    float sampleRate = 44100;
    float buffersize = 512;
    
    bool keyStates[128] = { false };
    
    float magnitudeLeft = 0;
    float magnitudeRight = 0;
    
    float magnitudeLeftIn = 0;
    float magnitudeRightIn = 0;
    
    long currentTime = 0;
    long lastTime = 0;
    
    int layer = 0;
    int cpuLoad = 0;
    int loads[10] = {0};
    int currentMeasure = 0;
    int currentSample = 0;
    
    bool isOptionPressed = false;
    
    Sampler* defaultSampler = nullptr;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
