/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 5.2.1

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library.
  Copyright (c) 2017 - ROLI Ltd.

  ==============================================================================
*/

#pragma once

//[Headers]     -- You can add your own extra header files here --
#include "../JuceLibraryCode/JuceHeader.h"
#include "MainTabbedComponent.h"
#include "ExtendedFileBrowser.h"
#include "EditorListener.h"
#include "InstrumentBrowser.h"
#include "PulsingTextButton.h"
#include "PropertyPanelLookAndFeel.h"

class Track;
class SynthEditor;

//[/Headers]

//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Projucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class PropertyView  : public juce::Component,
                      public juce::TimeSliceThread,
                      public juce::TimeSliceClient,
                      public EditorListener
{
public:
    //==============================================================================
    PropertyView ();
    ~PropertyView();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

    virtual int useTimeSlice() override;
    ExtendedFileBrowser* getBrowser();

    // Wired by MainComponent after both PropertyView and SynthEditor exist —
    // lets the Instruments tab create a virtual instrument track on
    // double-click without going through the plugin chooser dialog.
    void setEditor(SynthEditor* e) { editor = e; }
    InstrumentBrowser* getInstrumentBrowser() { return instrumentBrowser.get(); }

    // Highlight the "Show Plugin" button for a freshly created instrument
    // track. If a track is passed it also becomes the selected track so a
    // user clicking the pulsing button immediately opens the right editor.
    void pulseShowPluginButton (Track* trackToSelect = nullptr);
    //[/UserMethods]

    void paint (juce::Graphics& g) override;
    void resized() override;

    virtual void selectionChanged(juce::Component* m) override;
    virtual void fileChanged(String name) override;
    virtual void dirtyChanged(bool dirty) override {};
private:
    //[UserVariables]   -- You can add your own custom variables in this section.

    juce::PropertyPanel* propertyPanel = nullptr;
    std::unique_ptr<juce::TextEditor> descriptionEditor;
    juce::DirectoryContentsList* directoryContents = nullptr;
    juce::WildcardFileFilter* filter = nullptr;
    std::unique_ptr<ExtendedFileBrowser> browser = nullptr;
    FileBrowserModel* model = nullptr;
    std::unique_ptr<InstrumentBrowser> instrumentBrowser = nullptr;
    SynthEditor* editor = nullptr;

    // Owned by PropertyView, attached to propertyPanel via setLookAndFeel.
    // Must be cleared (setLookAndFeel(nullptr)) before the propertyPanel is
    // destroyed — handled in the destructor.
    std::unique_ptr<PropertyPanelLookAndFeel> propertyPanelLnf;
    //[/UserVariables]

    //==============================================================================
    std::unique_ptr<MainTabbedComponent> propertyTab = nullptr;
    std::unique_ptr<MainTabbedComponent> tabbedComponent = nullptr;

    std::unique_ptr<PulsingTextButton> showButton;
    
	Track* selectedTrack = nullptr;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PropertyView)
};

//[EndFile] You can add extra defines here...
//[/EndFile]
