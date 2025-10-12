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
#include "EditorComponent.h"
#include "DefaultToolbarItemFactory.h"
#include "ModuleBrowser.h"
#include "AudioEngine/ADSR.h"
#include "MidiProcessor.h"
#include "CommandParser.h"
#include "TransportDisplayComponent.h"
#include <math.h>
#include "BounceProgressDialog.h"
#include "Metronome.h"
#include "AudioLevelMeter.h"
#include "SpektrumAnalyzer.h"
#include "RotatingCubeComponent.h"

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

class MainComponent   : public juce::AudioAppComponent,
                        public juce::MenuBarModel,
                        public juce::KeyListener,
                        public juce::Button::Listener,
                        public juce::Timer,                        
                        // public juce::DragAndDropContainer,
                        public juce::ApplicationCommandManager,
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
    void startBounce(const juce::File& file, double duration);
    juce::String tryExecuteChatInstruction(const std::string& jsonString);

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
        DBG("Set bouncing", juce::String(b));
        bouncing.store(b);
    }

    bool isBouncing() {
        return bouncing.load();
    }


    TrackNavigator* navigator = nullptr;
    MidiProcessor* midiProcessor = nullptr;

private:
    //==============================================================================

    std::atomic<bool> bouncing{ false };
    std::atomic<bool> running{ false };
    void openSettings();
    
	SynthEditor* editor = nullptr;
    std::unique_ptr<MainTabbedComponent> tab = nullptr;
    std::unique_ptr<PropertyView> propertyView = nullptr    ;
    std::unique_ptr<juce::MenuBarComponent> menu = nullptr;
    std::unique_ptr<ModuleBrowser> moduleBrowser = nullptr;
    ToolbarComboBox* layerCombobox = nullptr;

    juce::Label* cpuLoadLabel = nullptr;
    juce::Slider* loadSlider = nullptr;

    std::shared_ptr<MixerPanel> mixerPanel = nullptr;
    juce::PopupMenu* pluginMenu = nullptr;
    
    std::unique_ptr<EditorComponent> editorView = nullptr;
    ImageButton* lockButton = nullptr;
    CommandParser* parser = nullptr;

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
    
    int currentOctave = 0;

    std::unique_ptr<DefaultToolbarItemFactory> toolbarFactory;
    std::unique_ptr<juce::Toolbar> toolbar = nullptr;
    
    bool initialized = false;   
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

    std::unique_ptr<ChatGPTInterface> chat = nullptr;    
	std::unique_ptr<TransportDisplayComponent> transportDisplay = nullptr;
    std::unique_ptr<BounceThread> bounceThread;
    std::atomic<double> bounceProgress{ 0.0 };
    std::unique_ptr<Metronome> metronome;
    juce::AudioBuffer<float> metronomeBuffer;
    
	std::unique_ptr<AudioLevelMeter> levelMeter = nullptr;
    std::unique_ptr<SpectrumAnalyzer> spectrumAnalyzer = nullptr;

	std::unique_ptr<RotatingCubeComponent> rotatingCube = nullptr;

    CriticalSection lock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
