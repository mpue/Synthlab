/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 5.3.1

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library.
  Copyright (c) 2017 - ROLI Ltd.

  ==============================================================================
*/

//[Headers] You can add your own extra header files here...
#include "Project.h"
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
    saveButton->setButtonText (TRANS("Save"));
    saveButton->addListener (this);

    saveButton->setBounds (336, 192, 104, 24);

    addAndMakeVisible (timeLabel = new Label ("timeLabel",
                                              TRANS("0:00")));
    timeLabel->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    timeLabel->setJustificationType (Justification::centredLeft);
    timeLabel->setEditable (false, false, false);
    timeLabel->setColour (TextEditor::textColourId, Colours::black);
    timeLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    timeLabel->setBounds (784, 192, 48, 24);

    addAndMakeVisible (vuSliderLeft = new Slider ("vuSliderLeft"));
    vuSliderLeft->setRange (0, 10, 0);
    vuSliderLeft->setSliderStyle (Slider::LinearBar);
    vuSliderLeft->setTextBoxStyle (Slider::NoTextBox, true, 80, 20);
    vuSliderLeft->setColour (Slider::backgroundColourId, Colour (0x00000000));

    vuSliderLeft->setBounds (552, 192, 208, 8);

    addAndMakeVisible (vuSliderRight = new Slider ("vuSliderRight"));
    vuSliderRight->setRange (0, 10, 0);
    vuSliderRight->setSliderStyle (Slider::LinearBar);
    vuSliderRight->setTextBoxStyle (Slider::NoTextBox, true, 80, 20);
    vuSliderRight->setColour (Slider::backgroundColourId, Colour (0x00000000));

    vuSliderRight->setBounds (552, 208, 208, 8);

    addAndMakeVisible (monitorButton = new ToggleButton ("new toggle button"));
    monitorButton->setButtonText (TRANS("Monitor"));
    monitorButton->addListener (this);

    monitorButton->setBounds (456, 192, 87, 24);

    addAndMakeVisible (gainSlider = new Slider ("gainSLider"));
    gainSlider->setRange (0, 1, 0.01);
    gainSlider->setSliderStyle (Slider::LinearHorizontal);
    gainSlider->setTextBoxStyle (Slider::TextBoxLeft, false, 80, 20);
    gainSlider->addListener (this);

    gainSlider->setBounds (840, 192, 240, 24);


    //[UserPreSize]
    //[/UserPreSize]

    setSize (1000, 400);


    //[Constructor] You can add your own custom stuff here..
    vuSliderLeft->setLookAndFeel(Project::getInstance()->getLookAndFeel());
    vuSliderRight->setLookAndFeel(Project::getInstance()->getLookAndFeel());
    //[/Constructor]
}

AudioRecorderPanel::~AudioRecorderPanel()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    vuSliderRight->setLookAndFeel(nullptr);
    vuSliderLeft->setLookAndFeel(nullptr);
    //[/Destructor_pre]

    component = nullptr;
    recordButton = nullptr;
    playButton = nullptr;
    saveButton = nullptr;
    timeLabel = nullptr;
    vuSliderLeft = nullptr;
    vuSliderRight = nullptr;
    monitorButton = nullptr;
    gainSlider = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void AudioRecorderPanel::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (Colour (0xff222222));

    //[UserPaint] Add your own custom painting code here..

    //[/UserPaint]
}

void AudioRecorderPanel::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    component->setBounds (0, 0, proportionOfWidth (1.0000f), proportionOfHeight (0.2000f));
    //[UserResized] Add your own custom resize handling here..



    if (getParentComponent() != nullptr) {
        setSize(proportionOfWidth (1.0000f), getParentHeight());
    }

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
        sendChangeMessage();
        //[/UserButtonCode_playButton]
    }
    else if (buttonThatWasClicked == saveButton)
    {
        //[UserButtonCode_saveButton] -- add your button handler code here..
        state = SAVE;
        sendChangeMessage();
        //[/UserButtonCode_saveButton]
    }
    else if (buttonThatWasClicked == monitorButton)
    {
        //[UserButtonCode_monitorButton] -- add your button handler code here..
        monitor = monitorButton->getToggleState();
        sendChangeMessage();
        //[/UserButtonCode_monitorButton]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}

void AudioRecorderPanel::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == gainSlider)
    {
        //[UserSliderCode_gainSlider] -- add your slider handling code here..
        gain = gainSlider->getValue();
        //[/UserSliderCode_gainSlider]
    }

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
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

void AudioRecorderPanel::setCurrentTime(juce::String time) {
    this->timeLabel->setText(time, juce::NotificationType::dontSendNotification);
}

void AudioRecorderPanel::setMagnitude(int channel, float value) {
    if (channel == 0) {
        this->magnitudeLeft = value;
    }
    else if (channel == 1) {
        this->magnitudeRight = value;
    }
    this->vuSliderLeft->setValue(magnitudeLeft);
    this->vuSliderRight->setValue(magnitudeRight);
}

bool AudioRecorderPanel::isMonitoring() {
    return monitor;
}

float AudioRecorderPanel::getGain() {
    return gain;
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
                 overlayOpacity="0.330" fixedSize="0" initialWidth="1000" initialHeight="400">
  <BACKGROUND backgroundColour="ff222222"/>
  <GENERICCOMPONENT name="new component" id="59344fe7f0e5ab69" memberName="component"
                    virtualName="" explicitFocusOrder="0" pos="0 0 100% 20%" class="AudioThumbnailComponent"
                    params="sampleRate, bufferSize"/>
  <TEXTBUTTON name="recordButton" id="323ad77bb3d5e053" memberName="recordButton"
              virtualName="" explicitFocusOrder="0" pos="16 192 150 24" buttonText="Record"
              connectedEdges="0" needsCallback="1" radioGroupId="0"/>
  <TEXTBUTTON name="playButton" id="102893fa94a0e9a1" memberName="playButton"
              virtualName="" explicitFocusOrder="0" pos="176 192 150 24" buttonText="Play"
              connectedEdges="0" needsCallback="1" radioGroupId="0"/>
  <TEXTBUTTON name="saveButton" id="27ca541dd30dbe79" memberName="saveButton"
              virtualName="" explicitFocusOrder="0" pos="336 192 104 24" buttonText="Save"
              connectedEdges="0" needsCallback="1" radioGroupId="0"/>
  <LABEL name="timeLabel" id="cc1a2ead9377d355" memberName="timeLabel"
         virtualName="" explicitFocusOrder="0" pos="784 192 48 24" edTextCol="ff000000"
         edBkgCol="0" labelText="0:00" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15.00000000000000000000"
         kerning="0.00000000000000000000" bold="0" italic="0" justification="33"/>
  <SLIDER name="vuSliderLeft" id="63d12841b3689d7f" memberName="vuSliderLeft"
          virtualName="" explicitFocusOrder="0" pos="552 192 208 8" bkgcol="0"
          min="0.00000000000000000000" max="10.00000000000000000000" int="0.00000000000000000000"
          style="LinearBar" textBoxPos="NoTextBox" textBoxEditable="0"
          textBoxWidth="80" textBoxHeight="20" skewFactor="1.00000000000000000000"
          needsCallback="0"/>
  <SLIDER name="vuSliderRight" id="7a227d46f0bc9f2d" memberName="vuSliderRight"
          virtualName="" explicitFocusOrder="0" pos="552 208 208 8" bkgcol="0"
          min="0.00000000000000000000" max="10.00000000000000000000" int="0.00000000000000000000"
          style="LinearBar" textBoxPos="NoTextBox" textBoxEditable="0"
          textBoxWidth="80" textBoxHeight="20" skewFactor="1.00000000000000000000"
          needsCallback="0"/>
  <TOGGLEBUTTON name="new toggle button" id="a9936f4520a76f1b" memberName="monitorButton"
                virtualName="" explicitFocusOrder="0" pos="456 192 87 24" buttonText="Monitor"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="0"/>
  <SLIDER name="gainSLider" id="86075a829b4898a6" memberName="gainSlider"
          virtualName="" explicitFocusOrder="0" pos="840 192 240 24" min="0.00000000000000000000"
          max="1.00000000000000000000" int="0.01000000000000000021" style="LinearHorizontal"
          textBoxPos="TextBoxLeft" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1.00000000000000000000" needsCallback="1"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]
