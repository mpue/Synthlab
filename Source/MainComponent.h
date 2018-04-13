/*
  ==============================================================================


  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "SynthEditor.h"
#include "PropertyView.h"
#include "MainTabbedComponent.h"
#include <map>
//==============================================================================

class MainComponent   : public AudioAppComponent, public MenuBarModel, public KeyListener, public Button::Listener
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
    void buttonClicked (Button*);
    
    class DefaultToolbarItemFactory   : public ToolbarItemFactory
    {
    public:
        DefaultToolbarItemFactory() {}
        
        //==============================================================================
        // Each type of item a toolbar can contain must be given a unique ID. These
        // are the ones we'll use in this demo.
        enum oolbarItemIds
        {
            doc_new         = 1,
        };
        
        void getAllToolbarItemIds (Array<int>& ids) override
        {
            // This returns the complete list of all item IDs that are allowed to
            // go in our toolbar. Any items you might want to add must be listed here. The
            // order in which they are listed will be used by the toolbar customisation panel.
            
            ids.add (doc_new);

        }
        
        void getDefaultItemSet (Array<int>& ids) override
        {
            // This returns an ordered list of the set of items that make up a
            // toolbar's default set. Not all items need to be on this list, and
            // items can appear multiple times (e.g. the separators used here).
            ids.add (doc_new);

        }
        
        ToolbarItemComponent* createItem (int itemId) override
        {
            switch (itemId)
            {
                case doc_new:           return new ToolbarButton(itemId,"New",getImage(itemId), getImage(itemId));
       
   
                default:                break;
            }
            
            return nullptr;
        }
        
        Drawable* getImage(int itemId) {
            DrawableImage* di = new DrawableImage();
            di->setImage(ImageCache::getFromMemory(BinaryData::sawtooth_png, BinaryData::sawtooth_pngSize));
            return di;
        }
        
    };
        



    
private:
    //==============================================================================

    void openSettings();
    
	SynthEditor* editor;
    MainTabbedComponent* tab;
    PropertyView* propertyView = nullptr    ;
    Viewport* view;
    MenuBarComponent* menu;
    
    virtual StringArray getMenuBarNames() override;
    virtual PopupMenu getMenuForIndex(int index, const String & menuName) override;
    virtual void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;
    
    StretchableLayoutManager stretchableManager;
    StretchableLayoutResizerBar resizerBar;

    std::map<int,int> keyCodeMidiNote;
    
    int currentOctave = 2;
    DefaultToolbarItemFactory* toolbarFactory;
    Toolbar* toolbar = nullptr;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
