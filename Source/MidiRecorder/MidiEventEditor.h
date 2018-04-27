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
#include "MidiRecorder.h"
//[/Headers]




//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Projucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class MidiEventEditor  : public Component,
                         public TableListBoxModel
{
public:
    //==============================================================================
    MidiEventEditor ();
    ~MidiEventEditor();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

    int getNumRows() override;
    void paintCell (Graphics&,
                    int rowNumber,
                    int columnId,
                    int width, int height,
                    bool rowIsSelected) override;

    void paintRowBackground (Graphics&,
                             int rowNumber,
                             int width, int height,
                             bool rowIsSelected) override;
    
    void setRecorder(MidiRecorder* recorder);

    
    void update();
    
    //[/UserMethods]

    void paint (Graphics& g) override;
    void resized() override;



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    TableListBox* table;
    MidiRecorder* recorder = nullptr;
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<Viewport> component;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiEventEditor)
};

//[EndFile] You can add extra defines here...
//[/EndFile]
