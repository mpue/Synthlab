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
#include "AudioThumbnailComponent.h"
//[/Headers]




//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Projucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class SampleEditor  : public Component
{
public:
    //==============================================================================
    SampleEditor (int buffersize, float samAudioFormatManager* manager);
    ~SampleEditor();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    //[/UserMethods]

    void paint (Graphics& g) override;
    void resized() override;



private:
    //[UserVariables]   -- You can add your own custom variables in this section.


    MidiKeyboardState state;
    MidiKeyboardComponent::Orientation orientation = MidiKeyboardComponent::Orientation::horizontalKeyboard;
    int buffersize;
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<MidiKeyboardComponent> midiKeyboard;
    ScopedPointer<AudioThumbnailComponent> component;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleEditor)
};

//[EndFile] You can add extra defines here...
//[/EndFile]
