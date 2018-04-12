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

class MainComponent   : public AudioAppComponent, public MenuBarModel, public KeyListener
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
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
