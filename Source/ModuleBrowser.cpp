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
#include "ModuleBrowserTable.h"
//[/Headers]

#include "ModuleBrowser.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
ModuleBrowser::ModuleBrowser ()
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (label = new Label ("new label",
                                          TRANS("Browse modules")));
    label->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    label->setJustificationType (Justification::centredLeft);
    label->setEditable (false, false, false);
    label->setColour (TextEditor::textColourId, Colours::black);
    label->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    label->setBounds (16, 16, 150, 24);

    addAndMakeVisible (searchText = new TextEditor ("searchText"));
    searchText->setMultiLine (false);
    searchText->setReturnKeyStartsNewLine (false);
    searchText->setReadOnly (false);
    searchText->setScrollbarsShown (true);
    searchText->setCaretVisible (true);
    searchText->setPopupMenuEnabled (true);
    searchText->setText (TRANS("Type your search here"));

    addAndMakeVisible (searchButton = new TextButton ("searchButton"));
    searchButton->setButtonText (TRANS("Search"));
    searchButton->addListener (this);

    addAndMakeVisible (table = new ModuleBrowserTable());
    table->setName ("new component");


    //[UserPreSize]
    //[/UserPreSize]

    setSize (600, 400);


    //[Constructor] You can add your own custom stuff here..
    shadow = new DropShadow(Colours::black,5, Point<int>(2,2));
    shadower = new DropShadower(*shadow);
    shadower->setOwner(this);

    table = new ModuleBrowserTable();
    table->setSize(400,400);


    //[/Constructor]
}

ModuleBrowser::~ModuleBrowser()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    label = nullptr;
    searchText = nullptr;
    searchButton = nullptr;

    //[Destructor]. You can add your own custom destruction code here..
    delete shadower;
    delete shadow;
    delete table;
    //[/Destructor]
}

//==============================================================================
void ModuleBrowser::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    shadow->drawForRectangle(g, Rectangle<int>(0,0,getWidth(),getHeight()));
    //[/UserPrePaint]

    g.fillAll (Colour (0xff323e44));

    //[UserPaint] Add your own custom painting code here..
    g.setColour(juce::Colours::lightgrey);
    g.drawRect(0,0,getWidth(),getHeight());
    //[/UserPaint]
}

void ModuleBrowser::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    searchText->setBounds (16, 48, proportionOfWidth (0.8000f), 24);
    searchButton->setBounds (proportionOfWidth (0.9792f) - proportionOfWidth (0.1304f), 48, proportionOfWidth (0.1304f), 24);
    table->setBounds (0, 80, proportionOfWidth (0.9f), proportionOfHeight (0.75f));
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void ModuleBrowser::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == searchButton)
    {
        //[UserButtonCode_searchButton] -- add your button handler code here..
        //[/UserButtonCode_searchButton]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...


//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="ModuleBrowser" componentName=""
                 parentClasses="public Component" constructorParams="" variableInitialisers=""
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330"
                 fixedSize="0" initialWidth="600" initialHeight="400">
  <BACKGROUND backgroundColour="ff323e44"/>
  <LABEL name="new label" id="66cb949aa6b597f2" memberName="label" virtualName=""
         explicitFocusOrder="0" pos="16 16 150 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Browse modules" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15.00000000000000000000" kerning="0.00000000000000000000"
         bold="0" italic="0" justification="33"/>
  <TEXTEDITOR name="searchText" id="124678ed16caa914" memberName="searchText"
              virtualName="" explicitFocusOrder="0" pos="16 48 80.018% 24"
              initialText="Type your search here" multiline="0" retKeyStartsLine="0"
              readonly="0" scrollbars="1" caret="1" popupmenu="1"/>
  <TEXTBUTTON name="searchButton" id="f6bb678a5ca16b64" memberName="searchButton"
              virtualName="" explicitFocusOrder="0" pos="97.892%r 48 13.016% 24"
              buttonText="Search" connectedEdges="0" needsCallback="1" radioGroupId="0"/>
  <GENERICCOMPONENT name="new component" id="bf32056d96a8c64f" memberName="table"
                    virtualName="" explicitFocusOrder="0" pos="-0.5Cc 80 97.984% 79.059%"
                    class="ModuleBrowserTable" params=""/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]
