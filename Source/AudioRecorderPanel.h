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
class AudioRecorderPanel  : public Component,
                            public ChangeBroadcaster,
                            public Button::Listener
{
public:
    //==============================================================================
    AudioRecorderPanel (float sampleRate, int bufferSize);
    ~AudioRecorderPanel();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    
    enum State {
        IDLE,
        RECORDING,
        PLAYING,
        SAVE,
    };
    
    //[/UserMethods]

    void paint (Graphics& g) override;
    void resized() override;
    void buttonClicked (Button* buttonThatWasClicked) override;

    State getState();
    void setState(AudioRecorderPanel::State state);
    void updateContent(AudioSampleBuffer* buffer, int numSamples);
    
private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<AudioThumbnailComponent> component;
    ScopedPointer<TextButton> recordButton;
    ScopedPointer<TextButton> playButton;
    ScopedPointer<TextButton> saveButton;

    State state = IDLE;
    int recordedSamples = 0;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioRecorderPanel)
};

//[EndFile] You can add extra defines here...
//[/EndFile]
