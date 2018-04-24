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
#include <map>
//==============================================================================

class MainComponent   : public AudioAppComponent,
                        public MenuBarModel,
                        public KeyListener,
                        public Button::Listener,
                        public Timer,
                        public AudioProcessorPlayer
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
    void createKeyMap();
    void buttonClicked (Button*) override;
    void handleIncomingMidiMessage (MidiInput* source, const MidiMessage& message) override;
    void sendGateMessage(Module* module, int velocity,int note,bool on);
    void sendNoteMessage(Module* module, int note);
    void sendControllerMessage(Module* module, int controller, float value);
    void processModule(Module* m);
    void audioDeviceIOCallback (const float** inputChannelData,
                                int numInputChannels,
                                float** outputChannelData,
                                int numOutputChannels,
                                int numSamples) override;
    int getNumActiveChannels(int i);
    
    class DefaultToolbarItemFactory   : public ToolbarItemFactory
    {
    public:
        DefaultToolbarItemFactory() {

        }
        
        ~DefaultToolbarItemFactory() {
        }
        
        //==============================================================================
        // Each type of item a toolbar can contain must be given a unique ID. These
        // are the ones we'll use in this demo.
        enum oolbarItemIds
        {
            doc_new         = 1,
            doc_open        = 2,
            doc_save        = 3,
            delete_element  = 4,
            app_settings    = 5,
            app_undo        = 6,
            app_redo        = 7,
            app_layer       = 8
            
        };
        
        void getAllToolbarItemIds (Array<int>& ids) override
        {
            // This returns the complete list of all item IDs that are allowed to
            // go in our toolbar. Any items you might want to add must be listed here. The
            // order in which they are listed will be used by the toolbar customisation panel.
            
            ids.add (doc_new);
            ids.add (doc_open);
            ids.add (doc_save);
            ids.add (delete_element);
            ids.add (app_settings);
            ids.add (app_undo);
            ids.add (app_redo);
            ids.add (app_layer);
        }
        
        void getDefaultItemSet (Array<int>& ids) override
        {
            // This returns an ordered list of the set of items that make up a
            // toolbar's default set. Not all items need to be on this list, and
            // items can appear multiple times (e.g. the separators used here).
            ids.add (doc_new);
            ids.add (doc_open);
            ids.add (doc_save);
            ids.add (delete_element);
            ids.add (app_settings);
            ids.add (app_undo);
            ids.add (app_redo);
            ids.add (app_layer);
        }
        
        ToolbarItemComponent* createItem (int itemId) override
        {
            
            ToolbarItemComponent* b;
            switch (itemId)
            {
                case doc_new:
                    b = new ToolbarButton(itemId,"New",getImage(itemId), getImage(itemId));
                    return b;
                case doc_open:
                    b = new ToolbarButton(itemId,"Open",getImage(itemId), getImage(itemId));
                    return b;
                case doc_save:
                    b = new ToolbarButton(itemId,"Save",getImage(itemId), getImage(itemId));
                    return b;
                case delete_element:
                    b = new ToolbarButton(itemId,"Delete",getImage(itemId), getImage(itemId));
                    return b;
                case app_settings:
                    b = new ToolbarButton(itemId,"Settings",getImage(itemId), getImage(itemId));
                    return b;
                case app_undo:
                    b = new ToolbarButton(itemId,"Undo",getImage(itemId), getImage(itemId));
                    return b;
                case app_redo:
                    b = new ToolbarButton(itemId,"Redo",getImage(itemId), getImage(itemId));
                    return b;
                case app_layer:
                    b = new ToolbarComboBox(itemId);
                    return b;
                default:
                    break;
            }
            
            return nullptr;
        }
        
        Drawable* getImage(int itemId) {
            
            DrawableImage* di = new DrawableImage();
            
            if (itemId == 1)
                di->setImage(ImageCache::getFromMemory(BinaryData::new_png, BinaryData::new_pngSize));
            else if(itemId == 2)
                di->setImage(ImageCache::getFromMemory(BinaryData::open_png, BinaryData::open_pngSize));
            else if(itemId == 3)
                di->setImage(ImageCache::getFromMemory(BinaryData::save_png, BinaryData::save_pngSize));
            else if(itemId == 4)
                di->setImage(ImageCache::getFromMemory(BinaryData::delete_png, BinaryData::delete_pngSize));
            else if(itemId == 5)
                di->setImage(ImageCache::getFromMemory(BinaryData::settings_png, BinaryData::settings_pngSize));
            else if(itemId == 6)
                di->setImage(ImageCache::getFromMemory(BinaryData::undo_png, BinaryData::undo_pngSize));
            else if(itemId == 7)
                di->setImage(ImageCache::getFromMemory(BinaryData::redo_png, BinaryData::redo_pngSize));
            return di;
        }
        
        int numItems() {
            return 8;
        }
        
    };
    
    void timerCallback() override;
    
private:
    //==============================================================================

    void openSettings();
    
	SynthEditor* editor = nullptr;
    MainTabbedComponent* tab = nullptr;
    PropertyView* propertyView = nullptr    ;
    Viewport* view = nullptr;
    MenuBarComponent* menu;
    

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
    bool running = false;
    
    // default settings just in case
    float sampleRate = 44100;
    float buffersize = 512;
    
    bool keyStates[128] = { false };
    
    float magnitudeLeft = 0;
    float magnitudeRight = 0;
    
    float magnitudeLeftIn = 0;
    float magnitudeRightIn = 0;
    
    int layer = 0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
