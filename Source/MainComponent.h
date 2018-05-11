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
#include "AudioEngine/ADSR.h"
#include <map>
//==============================================================================

class MainComponent   : public juce::AudioAppComponent,
                        public juce::MenuBarModel,
                        public juce::KeyListener,
                        public juce::Button::Listener,
                        public juce::Timer,
                        public juce::AudioProcessorPlayer,
                        public juce::DragAndDropContainer,
                        public ApplicationCommandManager
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
    
    void handleIncomingMidiMessage (juce::MidiInput* source, const juce::MidiMessage& message) override;
    void sendGateMessage(Module* module, int channel, int velocity,int note,bool on);
    void sendNoteMessage(Module* module, int channel, int note);
    void sendControllerMessage(Module* module, int channel, int controller, float value);
    void sendPitchBendMessage(Module *module, int channel, int pitch);
    void processModule(Module* m);
    int getNumActiveChannels(int i);
    
    void refreshMidiInputs();
    void enableAllMidiInputs();
    void disableAllMidiInputs();
    
    SynthEditor* getEditor() {
        return editor;
    }
    
    void mouseDrag (const juce::MouseEvent& event) override;
    virtual void dragOperationStarted (const juce::DragAndDropTarget::SourceDetails& details) override;
    void timerCallback() override;
    
private:
    //==============================================================================

    void openSettings();
    
	SynthEditor* editor = nullptr;
    MainTabbedComponent* tab = nullptr;
    PropertyView* propertyView = nullptr    ;
    juce::Viewport* view = nullptr;
    juce::MenuBarComponent* menu;
    ModuleBrowser* moduleBrowser = nullptr;

    juce::Label* cpuLoadLabel = nullptr;
    juce::Slider* loadSlider = nullptr;

    MixerPanel* mixerPanel = nullptr;
    Mixer* mixer = nullptr;
    juce::PopupMenu* pluginMenu = nullptr;
    
    EditorComponent* editorView = nullptr;
    
    virtual juce::StringArray getMenuBarNames() override;
    virtual juce::PopupMenu getMenuForIndex(int index, const juce::String & menuName) override;
    virtual void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;
    
    void createConfig();
    void createToolbar();
    void createMenu();
    void createCPUMeter();
    void createStudioLayout();
    void createPlayerLayout();
    
    juce::StretchableLayoutManager stretchableManager;
    juce::StretchableLayoutResizerBar resizerBar;
    
    std::map<int,int> keyCodeMidiNote;
    
    int currentOctave = 1;
    DefaultToolbarItemFactory* toolbarFactory;
    juce::Toolbar* toolbar = nullptr;
    
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
    
    StringArray sampleData;
    
    bool isOptionPressed = false;
    
    Sampler* defaultSampler = nullptr;

    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
