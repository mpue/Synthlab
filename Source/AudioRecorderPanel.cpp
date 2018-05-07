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

//[Headers] You can add your own extra header files here...
//[/Headers]

#include "AudioRecorderPanel.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
AudioRecorderPanel::AudioRecorderPanel (float sampleRate, int bufferSize)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (component = new AudioThumbnailComponent (sampleRate, bufferSize));
    component->setName ("new component");

    addAndMakeVisible (recordButton = new TextButton ("recordButton"));
    recordButton->setButtonText (TRANS("Record"));
    recordButton->addListener (this);

    recordButton->setBounds (16, 192, 150, 24);

    addAndMakeVisible (playButton = new TextButton ("playButton"));
    playButton->setButtonText (TRANS("Play"));
    playButton->addListener (this);

    playButton->setBounds (176, 192, 150, 24);

    addAndMakeVisible (saveButton = new TextButton ("saveButton"));
    saveButton->setButtonText (TRANS("Save recording"));
    saveButton->addListener (this);

    saveButton->setBounds (336, 192, 150, 24);


    //[UserPreSize]
    //[/UserPreSize]

    setSize (600, 400);


    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

AudioRecorderPanel::~AudioRecorderPanel()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    component = nullptr;
    recordButton = nullptr;
    playButton = nullptr;
    saveButton = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void AudioRecorderPanel::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (Colour (0xff323e44));

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void AudioRecorderPanel::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    component->setBounds (8, 8, proportionOfWidth (1.0000f), proportionOfHeight (0.2000f));
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void AudioRecorderPanel::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == recordButton)
    {
        //[UserButtonCode_recordButton] -- add your button handler code here..
        if (state == IDLE) {
            state = RECORDING;
            recordButton->setButtonText("Stop recording");
        }
        else {
            state = IDLE;
            recordButton->setButtonText("Start recording");
        }
        sendChangeMessage();
        
        //[/UserButtonCode_recordButton]
    }
    else if (buttonThatWasClicked == playButton)
    {
        //[UserButtonCode_playButton] -- add your button handler code here..
        if (state == IDLE) {
            state = PLAYING;
            playButton->setButtonText("Stop playback");
        }
        else {
            state = IDLE;
            playButton->setButtonText("Start playback");
        }
        sendChangeMessage();        //[/UserButtonCode_playButton]
    }
    else if (buttonThatWasClicked == saveButton)
    {
        //[UserButtonCode_saveButton] -- add your button handler code here..
        state = SAVE;
        sendChangeMessage();
        //[/UserButtonCode_saveButton]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...

AudioRecorderPanel::State AudioRecorderPanel::getState() {
    return state;
}

void AudioRecorderPanel::setState(AudioRecorderPanel::State state) {
    this->state = state;
}

void AudioRecorderPanel::updateContent(AudioSampleBuffer *buffer, int numSamples) {
    this->component->setSampleBuffer(buffer);
    this->component->setStartPosition(0);
    this->component->setEndPosition(numSamples);
}

//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="AudioRecorderPanel" componentName=""
                 parentClasses="public Component, public ChangeBroadcaster" constructorParams="float sampleRate, int bufferSize"
                 variableInitialisers="" snapPixels="8" snapActive="1" snapShown="1"
                 overlayOpacity="0.330" fixedSize="0" initialWidth="600" initialHeight="400">
  <BACKGROUND backgroundColour="ff323e44"/>
  <GENERICCOMPONENT name="new component" id="59344fe7f0e5ab69" memberName="component"
                    virtualName="" explicitFocusOrder="0" pos="8 8 100% 19.958%"
                    class="AudioThumbnailComponent" params="sampleRate, bufferSize"/>
  <TEXTBUTTON name="recordButton" id="323ad77bb3d5e053" memberName="recordButton"
              virtualName="" explicitFocusOrder="0" pos="16 192 150 24" buttonText="Record"
              connectedEdges="0" needsCallback="1" radioGroupId="0"/>
  <TEXTBUTTON name="playButton" id="102893fa94a0e9a1" memberName="playButton"
              virtualName="" explicitFocusOrder="0" pos="176 192 150 24" buttonText="Play"
              connectedEdges="0" needsCallback="1" radioGroupId="0"/>
  <TEXTBUTTON name="saveButton" id="27ca541dd30dbe79" memberName="saveButton"
              virtualName="" explicitFocusOrder="0" pos="336 192 150 24" buttonText="Save recording"
              connectedEdges="0" needsCallback="1" radioGroupId="0"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]
