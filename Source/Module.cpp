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
    for (std::vector<Pin*>::iterator it = pins.begin(); it != pins.end(); ++it) {
        delete *it;
    }


    
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
        g.setColour(juce::Colours::lightgrey);
    }
    else {
        g.setColour(juce::Colours::grey);
    }
    
    g.fillRect(0,0,getWidth(), getHeight());

	g.setColour(juce::Colours::black);
	g.drawRect(0, 0, getWidth(), getHeight());


    for (int i = 0; i < pins.size();i++) {

        if (pins.at(i)->selected) {
            g.setColour(juce::Colours::red);
        }
        else {
            g.setColour(juce::Colours::black);
        }

		if (pins.at(i)->direction == Pin::Direction::IN) {
			g.fillRect(pins.at(i)->x,pins.at(i)->y, 5, 10);
		}
		else {
			g.fillRect(pins.at(i)->x + 5, pins.at(i)->y, 5, 10);
		}
    }

	// g.drawFittedText(getName(), 40, 10, getWidth(), 20, juce::Justification::top, 2);

	Component::paint(g);

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

Module::Module(String name) : Module() {
	setName(name);

	repaint();
}


void Module::addPin(Pin::Direction direction) {

    Pin* p = new Pin();

	int numInputs = 0;
	int numOutputs = 0;

	for (int i = 0; i < pins.size(); i++) {
		if (pins.at(i)->direction == Pin::Direction::IN) {
			numInputs++;
		}
		else {
			numOutputs++;
		}
	}

	if (direction == Pin::Direction::IN) {
		p->x = 0;
	    p->y = 10 + numInputs * 20;
	}
	else {
		p->x = getWidth() - 10;
		p->y = 10 + numOutputs * 20;
	}

	p->direction = direction;

    pins.push_back(p);
}


Pin* Module::getSelectedPin() {

	for (int i = 0; i < pins.size(); i++) {
		if (pins.at(i)->selected) {
			return pins.at(i);
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

bool Module::isMouseOverPin(int pin, Point<int> pos) {

	if (pos.x >= pins.at(pin)->x + getX() && pos.x <= pins.at(pin)->x + getX() + 10 &&
        pos.y >= pins.at(pin)->y+ getY() &&  pos.y <= pins.at(pin)->y + getY() + 10) {
		return true;
	}

	return false;
}



Point<int> Module::getPinPosition(int i) {
	return Point<int>(pins.at(i)->x + getX(), pins.at(i)->x + getX());
}



long Module::getIndex() {
	return index;
}


void Module::setIndex(long index) {
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
