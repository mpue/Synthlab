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

#include "SamplePropertiesPanel.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
SamplePropertiesPanel::SamplePropertiesPanel ()
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (loadSampleButton = new TextButton ("loadSampleButton"));
    loadSampleButton->setButtonText (TRANS("Load sample"));
    loadSampleButton->addListener (this);

    loadSampleButton->setBounds (8, 8, 150, 24);

    addAndMakeVisible (sampleStartSlider = new Slider ("sampleStartSlider"));
    sampleStartSlider->setRange (0, 10, 1);
    sampleStartSlider->setSliderStyle (Slider::LinearHorizontal);
    sampleStartSlider->setTextBoxStyle (Slider::TextBoxLeft, false, 80, 20);
    sampleStartSlider->addListener (this);

    sampleStartSlider->setBounds (8, 72, 280, 24);

    addAndMakeVisible (startLabel = new Label ("startLabel",
                                               TRANS("Sample start\n")));
    startLabel->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    startLabel->setJustificationType (Justification::centredLeft);
    startLabel->setEditable (false, false, false);
    startLabel->setColour (TextEditor::textColourId, Colours::black);
    startLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    startLabel->setBounds (8, 40, 150, 24);

    addAndMakeVisible (endLabel = new Label ("endLabel",
                                             TRANS("Sample end\n")));
    endLabel->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    endLabel->setJustificationType (Justification::centredLeft);
    endLabel->setEditable (false, false, false);
    endLabel->setColour (TextEditor::textColourId, Colours::black);
    endLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    endLabel->setBounds (312, 40, 150, 24);

    addAndMakeVisible (sampleEndSlider = new Slider ("sampleEndSlider"));
    sampleEndSlider->setRange (0, 10, 1);
    sampleEndSlider->setSliderStyle (Slider::LinearHorizontal);
    sampleEndSlider->setTextBoxStyle (Slider::TextBoxLeft, false, 80, 20);
    sampleEndSlider->addListener (this);

    sampleEndSlider->setBounds (312, 72, 280, 24);

    addAndMakeVisible (loopToggleButton = new ToggleButton ("loopToggleButton"));
    loopToggleButton->setButtonText (TRANS("loopSample"));
    loopToggleButton->addListener (this);

    loopToggleButton->setBounds (472, 16, 104, 24);


    //[UserPreSize]
    //[/UserPreSize]

    setSize (600, 400);


    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

SamplePropertiesPanel::~SamplePropertiesPanel()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    loadSampleButton = nullptr;
    sampleStartSlider = nullptr;
    startLabel = nullptr;
    endLabel = nullptr;
    sampleEndSlider = nullptr;
    loopToggleButton = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void SamplePropertiesPanel::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (Colour (0xff323e44));

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void SamplePropertiesPanel::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void SamplePropertiesPanel::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == loadSampleButton)
    {
        //[UserButtonCode_loadSampleButton] -- add your button handler code here..
        openSample();
        //[/UserButtonCode_loadSampleButton]
    }
    else if (buttonThatWasClicked == loopToggleButton)
    {
        //[UserButtonCode_loopToggleButton] -- add your button handler code here..
        module->setLoop(loopToggleButton->getToggleState());        
        //[/UserButtonCode_loopToggleButton]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}

void SamplePropertiesPanel::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == sampleStartSlider)
    {
        //[UserSliderCode_sampleStartSlider] -- add your slider handling code here..
        //[/UserSliderCode_sampleStartSlider]
    }
    else if (sliderThatWasMoved == sampleEndSlider)
    {
        //[UserSliderCode_sampleEndSlider] -- add your slider handling code here..
        //[/UserSliderCode_sampleEndSlider]
    }

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...

void SamplePropertiesPanel::setModule(SamplerModule *module) {
    this->module = module;
    
}

void SamplePropertiesPanel::setThumbnail(AudioThumbnailComponent* thumbnail) {
    this->thumb = thumbnail;
}

void SamplePropertiesPanel::openSample() {
    
    FileChooser chooser("Select file to open", File::nonexistent, "*");
    
    if (chooser.browseForFileToOpen()) {
        
#if JUCE_IOS
        URL url = chooser.getURLResult();
        InputStream* is = url.createInputStream(false);
        
        module->loadSample(is);
        
        delete is;
        
        ScopedPointer<XmlElement> xml = XmlDocument(data).getDocumentElement();
#else
        File file = chooser.getResult();
        FileInputStream* is = new FileInputStream(file);
        module->setSamplePath(file.getFullPathName(),module->getCurrentSampler());
        module->loadSample(is, module->getCurrentSampler());
        if (module->getBuffer() != nullptr) {
            thumb->setSampleBuffer(module->getBuffer());
        }
        
        // delete is;
        
#endif
        
    }
}

void SamplePropertiesPanel::updateValues() {
    std::function<void(void)> changeLambda =
    [=]() {  loopToggleButton->setToggleState(module->isLoop(), juce::NotificationType::dontSendNotification); };
    juce::MessageManager::callAsync(changeLambda);
    
}

//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="SamplePropertiesPanel" componentName=""
                 parentClasses="public Component" constructorParams="" variableInitialisers=""
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330"
                 fixedSize="0" initialWidth="600" initialHeight="400">
  <BACKGROUND backgroundColour="ff323e44"/>
  <TEXTBUTTON name="loadSampleButton" id="a16960d6fb35b217" memberName="loadSampleButton"
              virtualName="" explicitFocusOrder="0" pos="8 8 150 24" buttonText="Load sample"
              connectedEdges="0" needsCallback="1" radioGroupId="0"/>
  <SLIDER name="sampleStartSlider" id="44fd26b4ee3614d2" memberName="sampleStartSlider"
          virtualName="" explicitFocusOrder="0" pos="8 72 280 24" min="0.00000000000000000000"
          max="10.00000000000000000000" int="1.00000000000000000000" style="LinearHorizontal"
          textBoxPos="TextBoxLeft" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1.00000000000000000000" needsCallback="1"/>
  <LABEL name="startLabel" id="71e7a2809e388c0" memberName="startLabel"
         virtualName="" explicitFocusOrder="0" pos="8 40 150 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Sample start&#10;" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15.00000000000000000000" kerning="0.00000000000000000000"
         bold="0" italic="0" justification="33"/>
  <LABEL name="endLabel" id="9c8285b56816945c" memberName="endLabel" virtualName=""
         explicitFocusOrder="0" pos="312 40 150 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Sample end&#10;" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15.00000000000000000000" kerning="0.00000000000000000000"
         bold="0" italic="0" justification="33"/>
  <SLIDER name="sampleEndSlider" id="d602ebbdcb49beb6" memberName="sampleEndSlider"
          virtualName="" explicitFocusOrder="0" pos="312 72 280 24" min="0.00000000000000000000"
          max="10.00000000000000000000" int="1.00000000000000000000" style="LinearHorizontal"
          textBoxPos="TextBoxLeft" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1.00000000000000000000" needsCallback="1"/>
  <TOGGLEBUTTON name="loopToggleButton" id="c99e9f970df31557" memberName="loopToggleButton"
                virtualName="" explicitFocusOrder="0" pos="472 16 104 24" buttonText="loopSample"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="0"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]
