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
#include "Cell.h"
#include "StepSequencerModule.h"
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Projucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class SequenceEditor  : public Component,
                        public MouseListener,
                        public AudioProcessorPlayer,
                        public Timer,
                        public Button::Listener
{
public:
    //==============================================================================
    SequenceEditor (StepSequencerModule* ssm);
    ~SequenceEditor();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    void handleIncomingMidiMessage (MidiInput* source, const MidiMessage& message) override;
    virtual void handlePartialSysexMessage (MidiInput* source,
                                            const uint8* messageData,
                                            int numBytesSoFar,
                                            double timestamp) override;
    void setDeviceManager(AudioDeviceManager* manager);
    uint8_t decodeSysex(uint8_t *sysex, uint8_t *data, uint8_t len);

    void sendConfig();

    void timerCallback() override;

    //[/UserMethods]

    void paint (Graphics& g) override;
    void resized() override;
    void buttonClicked (Button* buttonThatWasClicked) override;
    void mouseMove (const MouseEvent& e) override;
    void mouseDown (const MouseEvent& e) override;
    void mouseDrag (const MouseEvent& e) override;
    void mouseUp (const MouseEvent& e) override;
    bool keyPressed (const KeyPress& key) override;
    void modifierKeysChanged (const ModifierKeys& modifiers) override;



private:
    //[UserVariables]   -- You can add your own custom variables in this section.

    Cell grid[64][6];

    int mouseX;
    int mouseY;

    int selectedCol;
    int selectedRow;

    const int gridWidth = 64;
    const int gridHeight = 6;
    const int cellSize = 32;

    const String notes[127] = {"C","C#", "D", "D#", "E", "F", "F#", "G", "G#" , "A" ,"A#", "B"};

    bool showVelocity = false;
    bool optionPressed = false;

    AudioDeviceManager* deviceManager;
    StepSequencerModule* ssm;

    //[/UserVariables]

    //==============================================================================
    ScopedPointer<TextButton> receiveButton;
    ScopedPointer<TextButton> sendButton;

    

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequenceEditor)
};

//[EndFile] You can add extra defines here...
//[/EndFile]
