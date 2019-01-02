/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 4.3.0

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright (c) 2015 - ROLI Ltd.

  ==============================================================================
*/

//[Headers] You can add your own extra header files here...
#include "MasterChannelPanel.h"
#include "ImageToggleButton.h"
#include "../Project/Project.h"
#include "../Mixer.h"
#include "Session.h"
//[/Headers]

#include "TrackPropertyPanel.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
TrackPropertyPanel::TrackPropertyPanel ()
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (nameLabel = new Label ("nameLabel",
                                              TRANS("Name")));
    nameLabel->setFont (Font (15.00f, Font::plain));
    nameLabel->setJustificationType (Justification::centredLeft);
    nameLabel->setEditable (false, true, false);
    nameLabel->setColour (Label::textColourId, Colours::white);
    nameLabel->setColour (TextEditor::textColourId, Colours::black);
    nameLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));
    nameLabel->addListener (this);

    addAndMakeVisible (volumeViewSlider = new Slider ("volumeViewSlider"));
    volumeViewSlider->setRange (0, 1.5, 0.01);
    volumeViewSlider->setSliderStyle (Slider::LinearBar);
    volumeViewSlider->setTextBoxStyle (Slider::NoTextBox, true, 80, 20);
    volumeViewSlider->setColour (Slider::backgroundColourId, Colour (0xff747373));
    volumeViewSlider->setColour (Slider::thumbColourId, Colour (0xff1997ff));
    volumeViewSlider->addListener (this);

    addAndMakeVisible (volumeSlider = new Slider ("volumeSlider"));
    volumeSlider->setRange (0, 1, 0.01);
    volumeSlider->setSliderStyle (Slider::RotaryVerticalDrag);
    volumeSlider->setTextBoxStyle (Slider::NoTextBox, true, 80, 20);
    volumeSlider->setColour (Slider::thumbColourId, Colours::white);
    volumeSlider->setColour (Slider::rotarySliderFillColourId, Colours::steelblue);
    volumeSlider->setColour (Slider::rotarySliderOutlineColourId, Colour (0x7cffffff));
    volumeSlider->addListener (this);

    addAndMakeVisible (balanceSlider = new Slider ("balanceSlider"));
    balanceSlider->setRange (-1, 1, 0.02);
    balanceSlider->setSliderStyle (Slider::RotaryVerticalDrag);
    balanceSlider->setTextBoxStyle (Slider::NoTextBox, true, 80, 20);
    balanceSlider->setColour (Slider::rotarySliderFillColourId, Colours::steelblue);
    balanceSlider->setColour (Slider::rotarySliderOutlineColourId, Colour (0x7cffffff));
    balanceSlider->addListener (this);

    addAndMakeVisible (label2 = new Label ("new label",
                                           TRANS("Volume")));
    label2->setFont (Font (12.00f, Font::plain));
    label2->setJustificationType (Justification::centredLeft);
    label2->setEditable (false, false, false);
    label2->setColour (Label::textColourId, Colours::white);
    label2->setColour (Label::outlineColourId, Colour (0x00ffffff));
    label2->setColour (TextEditor::textColourId, Colours::black);
    label2->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (label3 = new Label ("new label",
                                           TRANS("Pan")));
    label3->setFont (Font (12.00f, Font::plain));
    label3->setJustificationType (Justification::centredLeft);
    label3->setEditable (false, false, false);
    label3->setColour (Label::textColourId, Colours::white);
    label3->setColour (TextEditor::textColourId, Colours::black);
    label3->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (muteButton = new ImageToggleButton());
    muteButton->setName ("muteButton");

    addAndMakeVisible (soloButton = new ImageToggleButton());
    soloButton->setName ("soloButton");

    addAndMakeVisible (recButton = new ImageToggleButton());
    recButton->setName ("recButton");

    addAndMakeVisible (inputCombo = new ComboBox ("inputCombo"));
    inputCombo->setEditableText (false);
    inputCombo->setJustificationType (Justification::centredLeft);
    inputCombo->setTextWhenNothingSelected (String());
    inputCombo->setTextWhenNoChoicesAvailable (TRANS("(no choices)"));
    inputCombo->addListener (this);

    addAndMakeVisible (monoButton = new ImageToggleButton());
    monoButton->setName ("monoButton");

    addAndMakeVisible (inputsLabel = new Label ("inputsLabel",
                                                TRANS("Input")));
    inputsLabel->setFont (Font (12.00f, Font::plain));
    inputsLabel->setJustificationType (Justification::centredLeft);
    inputsLabel->setEditable (false, false, false);
    inputsLabel->setColour (Label::textColourId, Colours::white);
    inputsLabel->setColour (Label::outlineColourId, Colour (0x00ffffff));
    inputsLabel->setColour (TextEditor::textColourId, Colours::black);
    inputsLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (outputCombo = new ComboBox ("outputCombo"));
    outputCombo->setEditableText (false);
    outputCombo->setJustificationType (Justification::centredLeft);
    outputCombo->setTextWhenNothingSelected (String());
    outputCombo->setTextWhenNoChoicesAvailable (TRANS("(no choices)"));
    outputCombo->addListener (this);

    addAndMakeVisible (inputsLabel2 = new Label ("inputsLabel",
                                                 TRANS("Output")));
    inputsLabel2->setFont (Font (12.00f, Font::plain));
    inputsLabel2->setJustificationType (Justification::centredLeft);
    inputsLabel2->setEditable (false, false, false);
    inputsLabel2->setColour (Label::textColourId, Colours::white);
    inputsLabel2->setColour (Label::outlineColourId, Colour (0x00ffffff));
    inputsLabel2->setColour (TextEditor::textColourId, Colours::black);
    inputsLabel2->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (channelCombo = new ComboBox ("channelCombo"));
    channelCombo->setEditableText (true);
    channelCombo->setJustificationType (Justification::centredRight);
    channelCombo->setTextWhenNothingSelected (String());
    channelCombo->setTextWhenNoChoicesAvailable (TRANS("(no choices)"));
    channelCombo->addItem (TRANS("1"), 1);
    channelCombo->addItem (TRANS("2"), 2);
    channelCombo->addItem (TRANS("3"), 3);
    channelCombo->addItem (TRANS("4"), 4);
    channelCombo->addItem (TRANS("5"), 5);
    channelCombo->addItem (TRANS("6"), 6);
    channelCombo->addItem (TRANS("7"), 7);
    channelCombo->addItem (TRANS("8"), 8);
    channelCombo->addItem (TRANS("9"), 9);
    channelCombo->addItem (TRANS("10"), 10);
    channelCombo->addItem (TRANS("11"), 11);
    channelCombo->addItem (TRANS("12"), 12);
    channelCombo->addItem (TRANS("13"), 13);
    channelCombo->addItem (TRANS("14"), 14);
    channelCombo->addItem (TRANS("15"), 15);
    channelCombo->addItem (TRANS("16"), 16);
    channelCombo->addListener (this);

    addAndMakeVisible (channelLabel = new Label ("channelLabel",
                                                 TRANS("Channel")));
    channelLabel->setFont (Font (12.00f, Font::plain));
    channelLabel->setJustificationType (Justification::centredLeft);
    channelLabel->setEditable (false, false, false);
    channelLabel->setColour (Label::textColourId, Colours::white);
    channelLabel->setColour (Label::outlineColourId, Colour (0x00ffffff));
    channelLabel->setColour (TextEditor::textColourId, Colours::black);
    channelLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));


    //[UserPreSize]

    constrainer = new TrackPropertyConstrainer(50,300);

    resizer = new ResizableEdgeComponent(this,constrainer,ResizableEdgeComponent::bottomEdge);
    addAndMakeVisible(resizer);
    //[/UserPreSize]

    setSize (150, 200);


    //[Constructor] You can add your own custom stuff here..
    setSize(150,Project::DEFAULT_TRACK_HEIGHT);
    // clf = Project::getInstance()->getLookAndFeel();
    // volumeViewSlider->setLookAndFeel(clf);
	addKeyListener(this);

    muteButton->setText("M");
    soloButton->setText("S");
    recButton->setText("R");
    monoButton->setText("O");

    muteButton->setImages (false, true, true,
                          ImageCache::getFromMemory (ImageToggleButton::round_button_png, ImageToggleButton::round_button_pngSize), 1.000f, Colour (0x00000000),
                             Image(), 1.000f, Colour (0x00000000),
                           ImageCache::getFromMemory (ImageToggleButton::round_button_pushed_png, ImageToggleButton::round_button_pushed_pngSize), 1.000f, Colours::steelblue);

    soloButton->setImages (false, true, true,
                           ImageCache::getFromMemory (ImageToggleButton::round_button_png, ImageToggleButton::round_button_pngSize), 1.000f, Colour (0x00000000),
                           Image(), 1.000f, Colour (0x00000000),
                           ImageCache::getFromMemory (ImageToggleButton::round_button_pushed_png, ImageToggleButton::round_button_pushed_pngSize), 1.000f, Colours::darkorange);


    recButton->setImages (false, true, true,
                           ImageCache::getFromMemory (ImageToggleButton::round_button_png, ImageToggleButton::round_button_pngSize), 1.000f, Colour (0x00000000),
                           Image(), 1.000f, Colour (0x00000000),
                           ImageCache::getFromMemory (ImageToggleButton::round_button_pushed_png, ImageToggleButton::round_button_pushed_pngSize), 1.000f, Colours::darkred);

    monoButton->setImages (false, true, true,
                          ImageCache::getFromMemory (ImageToggleButton::round_button_png, ImageToggleButton::round_button_pngSize), 1.000f, Colour (0x00000000),
                          Image(), 1.000f, Colour (0x00000000),
                          ImageCache::getFromMemory (ImageToggleButton::round_button_pushed_png, ImageToggleButton::round_button_pushed_pngSize), 1.000f, Colours::green);


    muteButton->getButton()->addListener(this);
    soloButton->getButton()->addListener(this);
    recButton->getButton()->addListener(this);
    monoButton->getButton()->addListener(this);

    channelCombo->setSelectedId(1);

    //[/Constructor]
}

TrackPropertyPanel::~TrackPropertyPanel()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    nameLabel = nullptr;
    volumeViewSlider = nullptr;
    volumeSlider = nullptr;
    balanceSlider = nullptr;
    label2 = nullptr;
    label3 = nullptr;
    muteButton = nullptr;
    soloButton = nullptr;
    recButton = nullptr;
    inputCombo = nullptr;
    monoButton = nullptr;
    inputsLabel = nullptr;
    outputCombo = nullptr;
    inputsLabel2 = nullptr;
    channelCombo = nullptr;
    channelLabel = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    delete resizer;
    delete constrainer;
    //[/Destructor]
}

//==============================================================================
void TrackPropertyPanel::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (Colour (0xff454545));

    //[UserPaint] Add your own custom painting code here..

	if (selected) {
		g.setColour(Colour(0xff454545).brighter());
		g.fillRect(0, 0, 150, getHeight());
	}
	else {
		g.setColour(Colour(0xff454545));
		g.fillRect(0, 0, 150, getHeight());
	}

	g.setColour(Colours::steelblue);
	g.drawLine(0, getHeight(), 150, getHeight(),0.5);

    //[/UserPaint]
}

void TrackPropertyPanel::resized()
{
    //[UserPreResize] Add your own custom resize code here..

    //[/UserPreResize]

    nameLabel->setBounds (8, 8, 136, 24);
    volumeViewSlider->setBounds (16, 88, 128, 8);
    volumeSlider->setBounds (16, 32, 32, 32);
    balanceSlider->setBounds (64, 32, 32, 32);
    label2->setBounds (8, 64, 55, 16);
    label3->setBounds (64, 64, 32, 16);
    muteButton->setBounds (16, 112, 24, 24);
    soloButton->setBounds (48, 112, 24, 24);
    recButton->setBounds (80, 112, 24, 24);
    inputCombo->setBounds (16, 160, 128, 24);
    monoButton->setBounds (112, 112, 24, 24);
    inputsLabel->setBounds (16, 144, 55, 16);
    outputCombo->setBounds (16, 208, 128, 24);
    inputsLabel2->setBounds (16, 192, 55, 16);
    channelCombo->setBounds (80, 240, 63, 24);
    channelLabel->setBounds (16, 244, 55, 16);
    //[UserResized] Add your own custom resize handling here..
    resizer->setBounds(0,getHeight()-5, getWidth(),5);


    if (track != NULL)
        track->setBounds(0, track->getY(), track->getWidth(), getHeight());

    /*

	if (getBounds().getHeight() < 150) {

        volumeViewSlider->setBounds(10, 36, 120,8);
        volumeViewSlider->setSliderStyle(juce::Slider::LinearBar);
		// volumeViewSlider->setVisible(false);
	}
	else {
        volumeViewSlider->setBounds(101, 32, 8, 120);
        volumeViewSlider->setSliderStyle(juce::Slider::LinearBarVertical);
		// volumeViewSlider->setVisible(true);
	}

    */


    //[/UserResized]
}

void TrackPropertyPanel::labelTextChanged (Label* labelThatHasChanged)
{
    //[UserlabelTextChanged_Pre]
    //[/UserlabelTextChanged_Pre]

    if (labelThatHasChanged == nameLabel)
    {
        //[UserLabelCode_nameLabel] -- add your label text handling code here..
        track->setName(nameLabel->getText());
        //[/UserLabelCode_nameLabel]
    }

    //[UserlabelTextChanged_Post]
    //[/UserlabelTextChanged_Post]
}

void TrackPropertyPanel::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == volumeViewSlider)
    {
        //[UserSliderCode_volumeViewSlider] -- add your slider handling code here..
        //[/UserSliderCode_volumeViewSlider]
    }
    else if (sliderThatWasMoved == volumeSlider)
    {
        //[UserSliderCode_volumeSlider] -- add your slider handling code here..
        track->setVolume(volumeSlider->getValue());
        //[/UserSliderCode_volumeSlider]
    }
    else if (sliderThatWasMoved == balanceSlider)
    {
        //[UserSliderCode_balanceSlider] -- add your slider handling code here..
        track->setPan(balanceSlider->getValue());
        //[/UserSliderCode_balanceSlider]
    }

    //[UsersliderValueChanged_Post]
    Mixer::getInstance()->sendChangeMessage();
    //[/UsersliderValueChanged_Post]
}

void TrackPropertyPanel::comboBoxChanged (ComboBox* comboBoxThatHasChanged)
{
    
    //[UsercomboBoxChanged_Pre]
    //[/UsercomboBoxChanged_Pre]

    if (comboBoxThatHasChanged == inputCombo)
    {
        //[UserComboBoxCode_inputCombo] -- add your combo box handling code here..

        if (this->track != NULL) {

            if (track->getType() == Track::Type::AUDIO) {
                if (track->isMono()) {
                    int input[2] =  { inputCombo->getSelectedId() - 1, inputCombo->getSelectedId() - 1 };
                    track->setInputChannels(input);
                }
                else {
                    int input[2] =  { inputCombo->getSelectedId() - 1, inputCombo->getSelectedId() };
                    track->setInputChannels(input);
                }
            }
            else if(track->getType() == Track::Type::MIDI) {
                if (Mixer::getInstance()->getMidiInputs().size() > 0) {
                    track->setMidiInputDevice(Mixer::getInstance()->getMidiInputs().at(inputCombo->getSelectedId() - 1));
                }
            }

        }

        //[/UserComboBoxCode_inputCombo]
    }
    else if (comboBoxThatHasChanged == outputCombo)
    {
        //[UserComboBoxCode_outputCombo] -- add your combo box handling code here..

        if (this->track != NULL) {
            if (track->getType() == Track::Type::AUDIO) {
                if (track->isMono()) {
                    int output[2] =  { outputCombo->getSelectedId() - 1, outputCombo->getSelectedId() - 1 };
                    track->setOutputChannels(output);
                }
                else {
                    int output[2] =  { outputCombo->getSelectedId() - 1, outputCombo->getSelectedId() };
                    track->setOutputChannels(output);
                }

            }
            else if(track->getType() == Track::Type::MIDI) {
                if (Mixer::getInstance()->getMidiOutputs().size() > 0) {
                    track->setMidiOutputDevice(Mixer::getInstance()->getMidiOutputs().at(outputCombo->getSelectedId() - 1));
                }
            }


        }
        //[/UserComboBoxCode_outputCombo]
    }
    else if (comboBoxThatHasChanged == channelCombo)
    {
        //[UserComboBoxCode_channelCombo] -- add your combo box handling code here..
        track->setMidiChannel(channelCombo->getSelectedId(), true);
        //[/UserComboBoxCode_channelCombo]
    }

    //[UsercomboBoxChanged_Post]
    //[/UsercomboBoxChanged_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
void TrackPropertyPanel::setName(juce::String name ) {
    if (this->nameLabel != nullptr)
        this->nameLabel->setText(name, juce::NotificationType::dontSendNotification);
}

void TrackPropertyPanel::setTrack(Track *track) {
    this->track = track;

    if (this->track->getType() == Track::Type::MIDI) {
        this->channelCombo->setSelectedId(track->getMidiChannel());
        this->channelCombo->setVisible(true);
        this->channelLabel->setVisible(false);
    }
    else {
        this->channelLabel->setVisible(false);
        this->channelCombo->setVisible(false);
    }

    this->volumeSlider->setValue(track->getVolume());
}

Track* TrackPropertyPanel::getTrack() {
    return this->track;
}

void TrackPropertyPanel::update() {
    this->volumeViewSlider->setValue(track->getMagnitude(0));
}

void TrackPropertyPanel::setSelected(bool selected)
{
	this->selected = selected;
	track->setSelected(this->selected);
	repaint();
}

bool TrackPropertyPanel::keyPressed(const KeyPress & key, Component * originatingComponent)
{
	if (key.getModifiers().shiftModifier) {
		Logger::getCurrentLogger()->writeToLog("Shift pressed");
	}

	return false;
}

bool TrackPropertyPanel::keyStateChanged(bool isKeyDown, Component * originatingComponent)
{
	// Logger::getCurrentLogger()->writeToLog("keyStateChanged to "+String(isKeyDown));
	return false;
}


void TrackPropertyPanel::changeListenerCallback(ChangeBroadcaster * source) {
    if (MasterChannelPanel* channel = dynamic_cast<MasterChannelPanel*>(source)){
        muteButton->setToggleState(false, juce::NotificationType::dontSendNotification);
    }
    else if (Mixer::getInstance() == source) {

        if (Mixer::getInstance()->getTracks().size() == 0) {
            return;
        }
        
        Track* t = Mixer::getInstance()->getLastModifiedTrack();

        if (track == t) {
            setName(t->getName());
            muteButton->setToggleState(t->isMute(), juce::NotificationType::dontSendNotification);
            soloButton->setToggleState(t->isSolo(), juce::NotificationType::dontSendNotification);
            volumeSlider->setValue(t->getVolume(),  juce::NotificationType::dontSendNotification);
            balanceSlider->setValue(t->getPan(),  juce::NotificationType::dontSendNotification);
        }

    }
}

void TrackPropertyPanel::updateChannels() {

    if (track->getType() == Track::Type::AUDIO) {
        StringArray channels = Mixer::getInstance()->getInputChannels();

        for (int i = 0; i < channels.size() - 1; i+=2) {
            inputCombo->addItem(channels.getReference(i) + " + " + channels.getReference(i + 1), i + 1);
        }

        channels = Mixer::getInstance()->getOutputChannels();

        for (int i = 0; i < channels.size() - 1; i+=2) {
            outputCombo->addItem(channels.getReference(i) + " + " + channels.getReference(i + 1), i + 1);
        }
    }
    else if (track->getType() == Track::Type::MIDI) {
        for (int i = 0; i < Mixer::getInstance()->getMidiInputs().size();i++) {
            inputCombo->addItem(Mixer::getInstance()->getMidiInputs().at(i), i + 1);
        }
        for (int i = 0; i < Mixer::getInstance()->getMidiOutputs().size();i++) {
            outputCombo->addItem(Mixer::getInstance()->getMidiOutputs().at(i), i + 1);
        }
    }

    if (inputCombo->getNumItems() > 0)
        inputCombo->setSelectedId(1);
    if (outputCombo->getNumItems() > 0)
        outputCombo->setSelectedId(1);


}

void TrackPropertyPanel::buttonClicked (Button* buttonThatWasClicked) {

    if (buttonThatWasClicked == muteButton->getButton()) {
        track->setMute(muteButton->getButton()->getToggleState());
    }
    else if(buttonThatWasClicked == soloButton->getButton()) {
        track->setSolo(soloButton->getButton()->getToggleState());
    }
    else if(buttonThatWasClicked == recButton->getButton()) {
        track->setRecording(recButton->getButton()->getToggleState());
    }
    else if(buttonThatWasClicked == monoButton->getButton()) {
        track->setMono(monoButton->getButton()->getToggleState());

        StringArray channels = Mixer::getInstance()->getInputChannels();

        inputCombo->clear();

        if (track->isMono()) {
            for (int i = 0; i < channels.size(); i++) {
                inputCombo->addItem(channels.getReference(i) + "("+ String(i) + ")", i + 1);
            }
        }
        else {
            for (int i = 0; i < channels.size() - 1; i+=2) {
                inputCombo->addItem(channels.getReference(i) + "|" + channels.getReference(i + 1)+ "("+ String(i) + ")" , i + 1);
            }
        }
        
        if (inputCombo->getNumItems() > 0)
            inputCombo->setSelectedId(1);

        channels = Mixer::getInstance()->getOutputChannels();

        outputCombo->clear();

        if (track->isMono()) {
            for (int i = 0; i < channels.size(); i++) {
                outputCombo->addItem(channels.getReference(i) + "("+ String(i) + ")", i + 1);
            }
        }
        else {
            for (int i = 0; i < channels.size() - 1; i+=2) {
                outputCombo->addItem(channels.getReference(i) + "|" + channels.getReference(i + 1)+ "("+ String(i) + ")" , i + 1);
            }
        }
        
        if (outputCombo->getNumItems() > 0)
            outputCombo->setSelectedId(1);

    }

    Mixer::getInstance()->sendChangeMessage();
}



//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="TrackPropertyPanel" componentName=""
                 parentClasses="public Component, public ChangeListener, public KeyListener, public ButtonListener"
                 constructorParams="" variableInitialisers="" snapPixels="8" snapActive="1"
                 snapShown="1" overlayOpacity="0.330" fixedSize="0" initialWidth="150"
                 initialHeight="200">
  <BACKGROUND backgroundColour="ff454545"/>
  <LABEL name="nameLabel" id="e816bfe76d3f902e" memberName="nameLabel"
         virtualName="" explicitFocusOrder="0" pos="8 8 136 24" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="Name" editableSingleClick="0"
         editableDoubleClick="1" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="33"/>
  <SLIDER name="volumeViewSlider" id="a57be949a5f89bda" memberName="volumeViewSlider"
          virtualName="" explicitFocusOrder="0" pos="16 88 128 8" bkgcol="ff747373"
          thumbcol="ff1997ff" min="0" max="1.5" int="0.010000000000000000208"
          style="LinearBar" textBoxPos="NoTextBox" textBoxEditable="0"
          textBoxWidth="80" textBoxHeight="20" skewFactor="1" needsCallback="1"/>
  <SLIDER name="volumeSlider" id="6d202def05db2b28" memberName="volumeSlider"
          virtualName="" explicitFocusOrder="0" pos="16 32 32 32" thumbcol="ffffffff"
          rotarysliderfill="ff4682b4" rotaryslideroutline="7cffffff" min="0"
          max="1" int="0.010000000000000000208" style="RotaryVerticalDrag"
          textBoxPos="NoTextBox" textBoxEditable="0" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1" needsCallback="1"/>
  <SLIDER name="balanceSlider" id="b241de49a09df77c" memberName="balanceSlider"
          virtualName="" explicitFocusOrder="0" pos="64 32 32 32" rotarysliderfill="ff4682b4"
          rotaryslideroutline="7cffffff" min="-1" max="1" int="0.020000000000000000416"
          style="RotaryVerticalDrag" textBoxPos="NoTextBox" textBoxEditable="0"
          textBoxWidth="80" textBoxHeight="20" skewFactor="1" needsCallback="1"/>
  <LABEL name="new label" id="85ebf37b370066e6" memberName="label2" virtualName=""
         explicitFocusOrder="0" pos="8 64 55 16" textCol="ffffffff" outlineCol="ffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="Volume" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="12" bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="b8f4f98ce397e313" memberName="label3" virtualName=""
         explicitFocusOrder="0" pos="64 64 32 16" textCol="ffffffff" edTextCol="ff000000"
         edBkgCol="0" labelText="Pan" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="12"
         bold="0" italic="0" justification="33"/>
  <GENERICCOMPONENT name="muteButton" id="5f1537d979acf69a" memberName="muteButton"
                    virtualName="ImageToggleButton" explicitFocusOrder="0" pos="16 112 24 24"
                    class="ImageToggleButton" params=""/>
  <GENERICCOMPONENT name="soloButton" id="2429c117450e5fa4" memberName="soloButton"
                    virtualName="ImageToggleButton" explicitFocusOrder="0" pos="48 112 24 24"
                    class="ImageToggleButton" params=""/>
  <GENERICCOMPONENT name="recButton" id="8a55a8aa642c1153" memberName="recButton"
                    virtualName="ImageToggleButton" explicitFocusOrder="0" pos="80 112 24 24"
                    class="ImageToggleButton" params=""/>
  <COMBOBOX name="inputCombo" id="8fa7ef2d51e455d3" memberName="inputCombo"
            virtualName="" explicitFocusOrder="0" pos="16 160 128 24" editable="0"
            layout="33" items="" textWhenNonSelected="" textWhenNoItems="(no choices)"/>
  <GENERICCOMPONENT name="monoButton" id="c1da9de25247e565" memberName="monoButton"
                    virtualName="ImageToggleButton" explicitFocusOrder="0" pos="112 112 24 24"
                    class="ImageToggleButton" params=""/>
  <LABEL name="inputsLabel" id="6e066caf6e46a50f" memberName="inputsLabel"
         virtualName="" explicitFocusOrder="0" pos="16 144 55 16" textCol="ffffffff"
         outlineCol="ffffff" edTextCol="ff000000" edBkgCol="0" labelText="Input"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="12" bold="0" italic="0" justification="33"/>
  <COMBOBOX name="outputCombo" id="5927994406d03619" memberName="outputCombo"
            virtualName="" explicitFocusOrder="0" pos="16 208 128 24" editable="0"
            layout="33" items="" textWhenNonSelected="" textWhenNoItems="(no choices)"/>
  <LABEL name="inputsLabel" id="42c156b7729e159e" memberName="inputsLabel2"
         virtualName="" explicitFocusOrder="0" pos="16 192 55 16" textCol="ffffffff"
         outlineCol="ffffff" edTextCol="ff000000" edBkgCol="0" labelText="Output"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="12" bold="0" italic="0" justification="33"/>
  <COMBOBOX name="channelCombo" id="9700632144ac1491" memberName="channelCombo"
            virtualName="" explicitFocusOrder="0" pos="80 240 63 24" editable="1"
            layout="34" items="1&#10;2&#10;3&#10;4&#10;5&#10;6&#10;7&#10;8&#10;9&#10;10&#10;11&#10;12&#10;13&#10;14&#10;15&#10;16"
            textWhenNonSelected="" textWhenNoItems="(no choices)"/>
  <LABEL name="channelLabel" id="d0368078a1a9d2b8" memberName="channelLabel"
         virtualName="" explicitFocusOrder="0" pos="16 244 55 16" textCol="ffffffff"
         outlineCol="ffffff" edTextCol="ff000000" edBkgCol="0" labelText="Channel"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="12" bold="0" italic="0" justification="33"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]
