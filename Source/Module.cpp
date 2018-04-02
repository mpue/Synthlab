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

#include "Module.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
Module::Module ()
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]


    //[UserPreSize]
    //[/UserPreSize]

    setSize (120, 140);


    //[Constructor] You can add your own custom stuff here..
    setInterceptsMouseClicks(false, false);
    //[/Constructor]
}

Module::~Module()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]



    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void Module::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (Colour (0xff323e44));

    //[UserPaint] Add your own custom painting code here..

    if (selected) {
        g.setColour(juce::Colours::lightblue);
    }
    else {
        g.setColour(juce::Colours::grey);
    }
    
    g.fillRect(5,0,getWidth()-10, getHeight());

    for (int i = 0; i < inputs.size();i++) {

        if (inputs.at(i).selected) {
            g.setColour(juce::Colours::red);
        }
        else {
            g.setColour(juce::Colours::blue);
        }

        g.fillEllipse(0,inputs.at(i).y, 10, 10);
    }

    for (int i = 0; i < outputs.size();i++) {

        if (outputs.at(i).selected) {
            g.setColour(juce::Colours::red);
        }
        else {
            g.setColour(juce::Colours::blue);
        }

        g.fillEllipse(getWidth() - 10 ,outputs.at(i).y, 10, 10);
    }


    //[/UserPaint]
}

void Module::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
void Module::addInput() {

    Pin p;

    p.x = 0;
    p.y = 10 + inputs.size() * 20;

    inputs.push_back(p);
}

void Module::addOutput() {

    Pin p;

    p.x = getWidth() - 10;
    p.y = 10 + outputs.size() * 20;

    outputs.push_back(p);
}

Pin* Module::getSelectedPin() {

	for (int i = 0; i < inputs.size(); i++) {
		if (inputs.at(i).selected) {
			return &inputs.at(i);
		}
	}

	for (int i = 0; i < outputs.size(); i++) {
		if (outputs.at(i).selected) {
			return &outputs.at(i);
		}
	}


    return nullptr;
}

void Module::setSelected(bool selected) {
    this->selected = selected;
}

bool Module::isSelected() {
    return selected;
}

bool Module::isMouseOverInput(int pin, Point<int> pos) {

	if (pos.x >= inputs.at(pin).x + getX() && pos.x <= inputs.at(pin).x + getX() + 10 && 
		pos.y >= inputs.at(pin).y+ getY() &&  pos.y <= inputs.at(pin).y + getY() + 10) {
		return true;
	}

	return false;
}

bool Module::isMouseOverOutput(int pin, Point<int> pos) {

	if (pos.x >= outputs.at(pin).x + getX() && pos.x <= outputs.at(pin).x + getX() + 10 &&
		pos.y >= outputs.at(pin).y + getY() && pos.y <= outputs.at(pin).y + getY() + 10) {
		return true;
	}

	return false;
}

Point<int> Module::getInputPinPosition(int i) {
	return Point<int>(inputs.at(i).x + getX(), inputs.at(i).x + getX());
}

Point<int> Module::getOutputPinPosition(int i) {
	return Point<int>(outputs.at(i).x + getX(), outputs.at(i).x + getX());
}

int Module::getIndex() {
	return index;
}


void Module::setIndex(int index) {
	this->index = index;
}

//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="Module" componentName=""
                 parentClasses="public Component" constructorParams="" variableInitialisers=""
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330"
                 fixedSize="0" initialWidth="120" initialHeight="140">
  <BACKGROUND backgroundColour="ff323e44"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]
