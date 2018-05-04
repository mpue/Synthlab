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
#include "Module.h"
#include "AudioModule.h"
#include "Connection.h"

//[/Headers]

#include "Module.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
Module::Module ()
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (nameEditor = new TextEditor ("nameEditor"));
    nameEditor->setMultiLine (false);
    nameEditor->setReturnKeyStartsNewLine (false);
    nameEditor->setReadOnly (false);
    nameEditor->setScrollbarsShown (false);
    nameEditor->setCaretVisible (true);
    nameEditor->setPopupMenuEnabled (true);
    nameEditor->setText (String());

    nameEditor->setBounds (0, 0, 120, 24);

    addAndMakeVisible (nameLabel = new Label ("nameLabel",
                                              getName()));
    nameLabel->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    nameLabel->setJustificationType (Justification::centredTop);
    nameLabel->setEditable (false, false, false);
    nameLabel->setColour (TextEditor::textColourId, Colours::black);
    nameLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    nameLabel->setBounds (0, 16, 120, 24);

    cachedImage_materialicons_301_viewmodule_64_0_ffffff_none_png_1 = ImageCache::getFromMemory (materialicons_301_viewmodule_64_0_ffffff_none_png, materialicons_301_viewmodule_64_0_ffffff_none_pngSize);

    //[UserPreSize]
    //[/UserPreSize]

    setSize (120, 140);


    //[Constructor] You can add your own custom stuff here..
    setInterceptsMouseClicks(false, false);
	nameEditor->setVisible(false);
    nameEditor->addListener(this);

    connections = new std::vector<Connection*>();
    modules = new std::vector<Module*>();
    pins.reserve(16);
    
    createProperties();
    
    //[/Constructor]
}

void Module::createProperties() {
    nameValue = new Value();
    nameListener = new NameListener(*nameValue, this);
}

Module::~Module()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    removeAllChangeListeners();
    delete nameListener;
    delete nameValue;
    //[/Destructor_pre]

    nameEditor = nullptr;
    nameLabel = nullptr;

    //[Destructor]. You can add your own custom destruction code here..
    for (std::vector<Pin*>::iterator it = pins.begin(); it != pins.end();) {
        
        for (std::vector<Pin*>::iterator it2 = (*it)->connections.begin(); it2 != (*it)->connections.end(); ) {
            if ((*it)->index == (*it2)->index) {
                it2 = (*it)->connections.erase(it2);
            }
            else {
                ++it2;
            }
            
        }
        
        delete *it;
        it = pins.erase(it);
    }
    
    

    for (std::vector<Connection*>::iterator it = connections->begin(); it != connections->end(); ) {
        delete *it;
        it = connections->erase(it);
    }
    for (std::vector<Module*>::iterator it = modules->begin(); it != modules->end();) {
        delete *it;
        it = modules->erase(it);
    }

    connections->clear();
    modules->clear();
    
    delete connections;
    delete modules;
    


    //[/Destructor]
}

//==============================================================================
void Module::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..



    // g.drawFittedText(getName(), 40, 10, getWidth(), 20, juce::Justification::top, 2);



    //[/UserPrePaint]

    g.fillAll (Colour (0xff323e44));

    {
        int x = 28, y = 44, width = 64, height = 64;
        //[UserPaintCustomArguments] Customize the painting arguments here..
        //[/UserPaintCustomArguments]
        g.setColour (Colours::black);
        g.drawImageWithin (cachedImage_materialicons_301_viewmodule_64_0_ffffff_none_png_1,
                           x, y, width, height,
                           RectanglePlacement::centred,
                           false);
    }

    //[UserPaint] Add your own custom painting code here..

    g.setColour(juce::Colours::grey);

    g.fillRect(0,0,getWidth(), getHeight());

    if (selected) {
        g.setColour(juce::Colours::orange);
    }
    else {
        g.setColour(juce::Colours::black);
    }
    g.drawRect(0, 0, getWidth(), getHeight());


    for (int i = 0; i < pins.size();i++) {

        if (pins.at(i)->isSelected()) {
            g.setColour(juce::Colours::white);
        }
        else {
            
            if(pins.at(i)->getType() == Pin::Type::AUDIO) {
                g.setColour(juce::Colours::orange);
            }
            else if (pins.at(i)->getType() == Pin::Type::VALUE) {
                 g.setColour(juce::Colours::yellow);
            }
            else {
                g.setColour(juce::Colours::lightblue);
            }
            
        }

        g.setFont(10);
        
        if (pins.at(i)->direction == Pin::Direction::IN) {
            //g.fillRect(pins.at(i)->x,pins.at(i)->y, 5, 10);
            g.drawEllipse(pins.at(i)->x,pins.at(i)->y, 10, 10,1);
            g.fillEllipse(pins.at(i)->x+2,pins.at(i)->y+2, 6, 6);
            g.drawText(pins.at(i)->getName() , pins.at(i)->x + 12,pins.at(i)->y, 10, 10, juce::Justification::left);
        }
        else {
            // g.fillRect(pins.at(i)->x + 5, pins.at(i)->y, 5, 10);
            g.drawEllipse(pins.at(i)->x, pins.at(i)->y, 10, 10,1);
            g.fillEllipse(pins.at(i)->x+2,pins.at(i)->y +2, 6, 6);
            g.drawText(pins.at(i)->getName() , pins.at(i)->x - 25,pins.at(i)->y, 20, 10, juce::Justification::right);
        }


    }

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

void Module::mouseDoubleClick (const MouseEvent& e)
{
    //[UserCode_mouseDoubleClick] -- Add your code here...
    setEditing(true);
    //[/UserCode_mouseDoubleClick]
}


//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...

Module::Module(String name) : Module() {
	setName(name);
	nameLabel->setText(name, NotificationType::dontSendNotification);
	repaint();
}

void Module::setEditing(bool editing) {
    /*
	if (editing) {
		nameLabel->setVisible(false);
		nameEditor->setVisible(true);
		nameEditor->setText(getName());
        setInterceptsMouseClicks(false,true);
        nameEditor->setWantsKeyboardFocus(true);
        nameEditor->setSelectAllWhenFocused(true);
        nameEditor->grabKeyboardFocus();
	}
	else {
		setName(nameEditor->getText());
        nameLabel->setText(getName(), NotificationType::dontSendNotification);
		nameLabel->setVisible(true);
		nameEditor->setVisible(false);
        setInterceptsMouseClicks(false,false);
        nameEditor->setWantsKeyboardFocus(false);
	}
     */
}

void Module::addPin(Pin* p) {
    pins.push_back(p);
    p->addChangeListener(this);
}

void Module::addPin(Pin::Direction direction, Pin* p) {

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
    p->addChangeListener(this);
    p->index = this->index * 100 + pins.size();
}

void Module::addPin(Pin::Direction direction) {

    Pin* p = new Pin(Pin::Type::AUDIO);

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
    p->addChangeListener(this);
    p->index = this->index * 100 + pins.size();
}


Pin* Module::getSelectedPin() {

    
	for (int i = 0; i < pins.size(); i++) {
		if (pins.at(i)->isSelected()) {
			return pins.at(i);
		}
	}
    

    return nullptr;
}

std::vector<Pin*>& Module::getPins() {
    return pins;
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

void Module::textEditorReturnKeyPressed(juce::TextEditor &) {
    setEditing(false);
}

void Module::setConnections(std::vector<Connection *>* connections) {
    this->connections = connections;
}

void Module::setModules(std::vector<Module *>* modules) {
    this->modules = modules;
}

std::vector<Connection*>* Module::getConnections() {
    return connections;
}

std::vector<Module*>* Module::getModules() {
    return modules;
}

void Module::setName(juce::String name) {
    Component::setName(name);
    nameLabel->setText(getName(), NotificationType::dontSendNotification);
}

void Module::eventReceived(Event *e) {

}

 juce::Array<PropertyComponent*>& Module::getProperties() {
    nameProp = new TextPropertyComponent(*nameValue,"Name",16, false,true);
    properties = juce::Array<PropertyComponent*>();
    properties.add(nameProp);
    return properties;
}

void Module::changeListenerCallback (ChangeBroadcaster* source) {

    AudioModule* am;
    
    if ((am = dynamic_cast<AudioModule*>(source)) != NULL) {
        Logger::writeToLog("Module "+getName()+" using sample rate "+String(sampleRate)+" kHz and buffer size of "+String(buffersize)+" bytes.");
        am->getSampleRate();
        buffersize = am->getBufferSize();
    }
}

void Module::process() {

    // if we have a terminal pin, just copy the value from the proxy to the output pin
    for (int i = 0; i < pins.size();i++){
        if (pins.at(i)->connections.size() == 1) {
            if (pins.at(i)->getType() == Pin::Type::VALUE && pins.at(i)->getTerminal() != nullptr) {
                pins.at(i)->getTerminal()->setValue(pins.at(i)->connections.at(0)->getValue());
            }
        }

    }
    
}

void Module::setSampleRate(float rate) {
    this->sampleRate = rate;
}

void Module::setBuffersize(int buffersize){
    this->buffersize = buffersize;
}

void Module::savePosition() {
    this->savedPosition.x = getX();
    this->savedPosition.y = getY();
}

Point<int> Module::getSavedPosition() {
    return savedPosition;
}

void Module::moveTo(int x, int y) {
    setTopLeftPosition(savedPosition.x + x, savedPosition.y + y);
}

String Module::getCategory() {
    return "none";
}

String Module::getDescription() {
    return "not available";
}

//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="Module" componentName=""
                 parentClasses="public Component, public TextEditor::Listener, public EventListener"
                 constructorParams="" variableInitialisers="" snapPixels="8" snapActive="1"
                 snapShown="1" overlayOpacity="0.330" fixedSize="0" initialWidth="120"
                 initialHeight="140">
  <METHODS>
    <METHOD name="mouseDoubleClick (const MouseEvent&amp; e)"/>
  </METHODS>
  <BACKGROUND backgroundColour="ff323e44">
    <IMAGE pos="28 44 64 64" resource="materialicons_301_viewmodule_64_0_ffffff_none_png"
           opacity="1.00000000000000000000" mode="1"/>
  </BACKGROUND>
  <TEXTEDITOR name="nameEditor" id="c38a5dbf58fc5656" memberName="nameEditor"
              virtualName="" explicitFocusOrder="0" pos="0 16 120 24" initialText=""
              multiline="0" retKeyStartsLine="0" readonly="0" scrollbars="0"
              caret="1" popupmenu="1"/>
  <LABEL name="nameLabel" id="61c171a5486b2448" memberName="nameLabel"
         virtualName="" explicitFocusOrder="0" pos="0 16 120 24" edTextCol="ff000000"
         edBkgCol="0" labelText="unnamed" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15.00000000000000000000"
         kerning="0.00000000000000000000" bold="0" italic="0" justification="12"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif

//==============================================================================
// Binary resources - be careful not to edit any of these sections!

// JUCER_RESOURCE: materialicons_301_viewmodule_64_0_ffffff_none_png, 335, "../../../../Downloads/material-icons_3-0-1_view-module_64_0_ffffff_none.png"
static const unsigned char resource_Module_materialicons_301_viewmodule_64_0_ffffff_none_png[] = { 137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,64,0,0,0,64,8,3,0,0,0,157,183,129,236,0,0,0,105,80,
76,84,69,0,0,0,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,252,27,173,118,0,0,0,34,116,82,78,83,0,2,3,6,7,9,12,17,20,21,25,26,32,33,36,39,42,49,56,60,64,71,75,94,100,111,133,136,140,145,171,183,197,241,75,188,238,113,0,0,0,115,73,68,65,84,
88,195,237,151,185,14,130,0,20,4,7,148,75,84,14,185,81,57,254,255,35,109,73,182,181,34,59,229,20,83,109,242,242,192,156,129,160,20,46,64,33,54,1,50,177,119,226,93,120,2,155,216,1,152,196,174,14,56,224,
128,3,14,28,2,209,34,228,192,87,108,11,244,98,223,103,184,76,97,35,68,192,75,236,13,120,136,173,60,36,7,28,112,192,129,127,6,174,31,33,5,102,177,53,208,137,29,253,113,25,128,31,3,127,77,21,129,60,2,142,
0,0,0,0,73,69,78,68,174,66,96,130,0,0};

const char* Module::materialicons_301_viewmodule_64_0_ffffff_none_png = (const char*) resource_Module_materialicons_301_viewmodule_64_0_ffffff_none_png;
const int Module::materialicons_301_viewmodule_64_0_ffffff_none_pngSize = 335;


//[EndFile] You can add extra defines here...
//[/EndFile]
