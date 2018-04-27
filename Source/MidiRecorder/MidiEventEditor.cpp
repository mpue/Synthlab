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

#include "MidiEventEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
MidiEventEditor::MidiEventEditor ()
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (component = new Viewport());

    //[UserPreSize]
    //[/UserPreSize]

    setSize (600, 400);


    //[Constructor] You can add your own custom stuff here..
    table = new TableListBox();
    table->getHeader().addColumn("Event type", 1, getWidth() / 5);
    table->getHeader().addColumn("Channel", 2, getWidth() / 5);
    table->getHeader().addColumn("Value", 3, getWidth() / 5);
    table->getHeader().addColumn("Velocity", 4, getWidth() / 5);
    table->getHeader().addColumn("Timestamp", 5, getWidth() / 2);
    table->setSize(getWidth(), getHeight());
    table->setModel(this);
    component->setViewedComponent(table);
    //[/Constructor]
}

MidiEventEditor::~MidiEventEditor()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    component = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void MidiEventEditor::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (Colour (0xff323e44));

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void MidiEventEditor::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]


    component->setBounds (8, 8, proportionOfWidth (0.9900f), proportionOfHeight (0.9900f));
    if (table != nullptr)
        table->setSize(component->getWidth(), component->getHeight());
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...

int MidiEventEditor::getNumRows() {
    if (recorder != nullptr) {
        return (int)recorder->getMessages().size();
    }
}

void MidiEventEditor::paintCell(juce::Graphics &g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) {
    g.setColour(juce::Colours::black);
    
    String text = "";
    
    if (columnId == 1) {
        
        String type = "unknown";
        
        if (recorder->getMessages().at(rowNumber).isNoteOn()) {
            type = "Note on";
        }
        else if (recorder->getMessages().at(rowNumber).isNoteOff()) {
            type = "Note off";
        }
        else if (recorder->getMessages().at(rowNumber).isSysEx()) {
            type = "Sysex";
        }
        else if (recorder->getMessages().at(rowNumber).isController()) {
            type = "Controller "+String(recorder->getMessages().at(rowNumber).getControllerNumber());
        }
        
        text = type;
    }
    else if (columnId == 2) {
        text = String(recorder->getMessages().at(rowNumber).getChannel());
    }
    else if (columnId == 3) {
        
        if (recorder->getMessages().at(rowNumber).isNoteOn() || recorder->getMessages().at(rowNumber).isNoteOff()) {
            text = String(recorder->getMessages().at(rowNumber).getNoteNumber());
        }
        else if (recorder->getMessages().at(rowNumber).isSysEx()) {
        }
        else if (recorder->getMessages().at(rowNumber).isController()) {
            text = String(recorder->getMessages().at(rowNumber).getControllerValue());
        }
    }
    else if (columnId == 4) {
        if (recorder->getMessages().at(rowNumber).isNoteOn() || recorder->getMessages().at(rowNumber).isNoteOff()) {
            text = String(recorder->getMessages().at(rowNumber).getVelocity());
        }
    }
    else if (columnId == 5) {
        text = String(recorder->getMessages().at(rowNumber).getTimeStamp());
    }
    g.drawText(text, 0,0, width,height, juce::Justification::centredLeft);
}

void MidiEventEditor::paintRowBackground(juce::Graphics &g, int rowNumber, int width, int height, bool rowIsSelected) {
    
    if (rowNumber % 2 == 0) {
        g.setColour(juce::Colours::grey);
    }
    else {
        g.setColour(juce::Colours::lightgrey);
    }
    g.fillRect(0,0,width,height);
}

void MidiEventEditor::setRecorder(MidiRecorder *recorder) {
    this->recorder = recorder;
}

void MidiEventEditor::update() {
    table->updateContent();
}

//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="MidiEventEditor" componentName=""
                 parentClasses="public Component, public TableListBoxModel" constructorParams=""
                 variableInitialisers="" snapPixels="8" snapActive="1" snapShown="1"
                 overlayOpacity="0.330" fixedSize="0" initialWidth="600" initialHeight="400">
  <BACKGROUND backgroundColour="ff323e44"/>
  <JUCERCOMP name="" id="29e7e0138fb2d15d" memberName="component" virtualName="Viewport"
             explicitFocusOrder="0" pos="8 8 98.972% 98.95%" sourceFile=""
             constructorParams=""/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]
