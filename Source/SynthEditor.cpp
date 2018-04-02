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

#include "SynthEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
SynthEditor::SynthEditor ()
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]


    //[UserPreSize]
    //[/UserPreSize]

    setSize (1280, 800);


    //[Constructor] You can add your own custom stuff here..

    Module* m1 = new Module();
	m1->setName("ADSR");
	m1->setIndex(0);
    m1->setTopLeftPosition(100,100);
	m1->addPin(Pin::Direction::IN);
	m1->addPin(Pin::Direction::IN);
	m1->addPin(Pin::Direction::IN);
	m1->addPin(Pin::Direction::OUT);

    addAndMakeVisible(m1);

    Module* m2 = new Module();
	m2->setName("OSC 1");
    m2->setTopLeftPosition(300,100);
	m2->setIndex(1);
	m2->addPin(Pin::Direction::IN);
	m2->addPin(Pin::Direction::IN);
	m2->addPin(Pin::Direction::OUT);

    addAndMakeVisible(m2);

	Module* m3 = new Module();
	m3->setName("Macro");
	m3->setTopLeftPosition(500, 100);
	m3->setIndex(2);
	m3->addPin(Pin::Direction::IN);
	m3->addPin(Pin::Direction::OUT);

	addAndMakeVisible(m3);

    modules.push_back(m1);
    modules.push_back(m2);
	modules.push_back(m3);

	setRepaintsOnMouseActivity(true);

    //[/Constructor]
}

SynthEditor::~SynthEditor()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]



    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void SynthEditor::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (Colour (0xff323e44));

    //[UserPaint] Add your own custom painting code here..

    g.setColour(juce::Colours::white);

	if (isLeftMouseDown) {

		if (lineStopX > 0 && lineStopY > 0) {
			g.drawLine(lineStartX, lineStartY, lineStopX, lineStopY);		
		}

	}

	for (int i = 0; i < connections.size(); i++) {
		Connection* c = connections.at(i);

		g.drawLine(c->source->getX() + c->a->x , c->source->getY() + c->a->y + 5, c->target->getX() + c->b->x , c->target->getY() + c->b->y + 5);

	}


    //[/UserPaint]
}

void SynthEditor::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void SynthEditor::mouseMove (const MouseEvent& e)
{
    //[UserCode_mouseMove] -- Add your code here...

	for (int i = 0; i < modules.size(); i++) {

		Module* m = modules.at(i);
		checkForPinSelection(e, m);

	}

    //[/UserCode_mouseMove]
}

void SynthEditor::mouseDown (const MouseEvent& e)
{
    //[UserCode_mouseDown] -- Add your code here...

	if (e.mods.isLeftButtonDown()) {
		isLeftMouseDown = true;
		minrepaintx = e.getPosition().x;
		minrepainty = e.getPosition().y;
	}

    
    for (int i = 0; i < modules.size(); i++) {
        
        if (modules.at(i)->getBounds().contains(e.x,e.y)) {
            modules.at(i)->setSelected(true);
			dragStartX = modules.at(i)->getX();
			dragStartY = modules.at(i)->getY();

        }
        else {
            modules.at(i)->setSelected(false);
        }
            
        
    }



    //[/UserCode_mouseDown]
}

void SynthEditor::mouseDrag (const MouseEvent& e)
{
    //[UserCode_mouseDrag] -- Add your code here...

	

	lineStartX = e.getMouseDownPosition().x;
	lineStartY = e.getMouseDownPosition().y;

	lineStopX = lineStartX + e.getDistanceFromDragStartX();
	lineStopY = lineStartY + e.getDistanceFromDragStartY();

	for (int i = 0; i < modules.size(); i++) {

		Module* m = modules.at(i);

		if (m->isSelected()) {

			if (m->getSelectedPin() == nullptr) {
				m->setTopLeftPosition(e.getDistanceFromDragStartX() + dragStartX, e.getDistanceFromDragStartY() + dragStartY);
				lineStopX = 0;
				lineStopY = 0;
				repaint();
			}

		}
		else {
			checkForPinSelection(e, m);
		}
	}



	/*
	int x, y, w, h;

	if (e.getPosition().x >= e.getMouseDownPosition().x) {
		x = e.getMouseDownPosition().x;
	}
	else {
		x = e.getPosition().x;
	}

	if (e.getPosition().y >= e.getMouseDownPosition().y) {
		y = e.getMouseDownPosition().y;
	}
	else {
		y = e.getPosition().y;
	}

	w = abs(e.getMouseDownPosition().x - e.getPosition().x);
	h = abs(e.getMouseDownPosition().y - e.getPosition().y);

	if (x <= minrepaintx) {
		minrepaintx = x;
	}

	if (y <= minrepainty) {
		minrepainty = y;
	}


	if (w >= maxrepaintw) {
		maxrepaintw = w;
	}

	if (h >= maxrepainth) {
		maxrepainth = h;
	}

	repaint(minrepaintx,minrepainty,maxrepaintw,maxrepainth);

	*/

	repaint();

    //[/UserCode_mouseDrag]
}

void SynthEditor::mouseUp (const MouseEvent& e)
{
    //[UserCode_mouseUp] -- Add your code here...
	if (e.mods.isLeftButtonDown()) {
		isLeftMouseDown = false;
	}

	for (int i = 0; i < modules.size(); i++) {

		Module* m = modules.at(i);

		if (m->isSelected()) {
			addConnection(e, m);
		}
	}

	lineStopX = 0;
	lineStopY = 0;

    //[/UserCode_mouseUp]
}

bool SynthEditor::keyPressed (const KeyPress& key)
{
    //[UserCode_keyPressed] -- Add your code here...
    return false;  // Return true if your handler uses this key event, or false to allow it to be passed-on.
    //[/UserCode_keyPressed]
}

void SynthEditor::modifierKeysChanged (const ModifierKeys& modifiers)
{
    //[UserCode_modifierKeysChanged] -- Add your code here...
    isAltDown = modifiers.isAltDown();
    //[/UserCode_modifierKeysChanged]
}

void SynthEditor::checkForPinSelection(const MouseEvent& e, Module* m) {

	for (int j = 0; j < m->pins.size(); j++) {

		if (m->isMouseOverPin(j, e.getPosition())) {
			m->pins.at(j).selected = true;
		}
		else {
			m->pins.at(j).selected = false;
		}

	}

	for (int j = 0; j < m->outputs.size(); j++) {

		if (m->isMouseOverPin(j, e.getPosition())) {
			m->outputs.at(j).selected = true;
		}
		else {
			m->outputs.at(j).selected = false;

		}

	}

	m->repaint();

}

void SynthEditor::addConnection(const MouseEvent& e, Module* source) {

	Pin* a = nullptr;
	Pin* b = nullptr;
	Module* target = nullptr;

	// find the selected input of the source module
	for (int j = 0; j < source->pins.size(); j++) {

		if (source->pins.at(j).selected) {
			a = &source->pins.at(j);
			break;
		}

	}

	// now find the target module
	for (int i = 0; i < modules.size(); i++) {

		Module* m = modules.at(i);

		if (m->getIndex() == source->getIndex()) {
			continue;
		}

		// find the selected input of the target  module
		for (int j = 0; j < m->pins.size(); j++) {

			if (m->pins.at(j).selected) {
				b = &m->pins.at(j);
				target = m;
				break;
			}

		}
	
	}

	if (source != nullptr && target != nullptr && a != nullptr && b != nullptr) {
		Connection* c = new Connection(source, a, target, b);
		connections.push_back(c);
		repaint();
	}

}


//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="SynthEditor" componentName=""
                 parentClasses="public Component" constructorParams="" variableInitialisers=""
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330"
                 fixedSize="0" initialWidth="1280" initialHeight="800">
  <METHODS>
    <METHOD name="mouseDrag (const MouseEvent&amp; e)"/>
    <METHOD name="mouseDown (const MouseEvent&amp; e)"/>
    <METHOD name="mouseUp (const MouseEvent&amp; e)"/>
    <METHOD name="keyPressed (const KeyPress&amp; key)"/>
    <METHOD name="modifierKeysChanged (const ModifierKeys&amp; modifiers)"/>
    <METHOD name="mouseMove (const MouseEvent&amp; e)"/>
  </METHODS>
  <BACKGROUND backgroundColour="ff323e44"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]

