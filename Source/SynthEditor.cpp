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
#include <map>

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


	setRepaintsOnMouseActivity(true);
	setMouseClickGrabsKeyboardFocus(true);
	setWantsKeyboardFocus(true);
    //[/Constructor]
}

SynthEditor::~SynthEditor()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]



    //[Destructor]. You can add your own custom destruction code here..

	for (std::vector<Connection*>::iterator it = connections.begin(); it != connections.end(); ++it) {
		delete *it;
	}
	for (std::vector<Module*>::iterator it = modules.begin(); it != modules.end(); ++it) {
		delete *it;
	}


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

		int x1 = c->source->getX() + c->a->x;
		int y1 = c->source->getY() + c->a->y + 5;
		int x2 = c->target->getX() + c->b->x;
		int y2 = c->target->getY() + c->b->y + 5;
		/*
		Path p = Path();

		// p.addLineSegment(Line<float>(c->source->getX() + c->a->x, c->source->getY() + c->a->y + 5, c->target->getX() + c->b->x, c->target->getY() + c->b->y + 5),1.0f);


		p.addLineSegment(Line<float>(x1, y1, x2, y2), 1.0f);

		int xm = (x1 + x2) / 2;
		int ym = (y1 + y2) / 2 + 2 ;

		p.cubicTo(Point<float>(x1,y1), Point<float>(xm,ym),Point<float>(x2,y2));


		PathStrokeType(50.0, juce::PathStrokeType::JointStyle::curved,
			juce::PathStrokeType::EndCapStyle::rounded)
			.createStrokedPath(p, p);


		g.strokePath(p,PathStrokeType(1));

		*/


		if (c->selected) {
			g.setColour(juce::Colours::cyan);
		}
		else {
			g.setColour(juce::Colours::white);
		}

		Logger::writeToLog("Connection : " + String(x1) + "," + String(y1) + "," + String(x2) + "," + String(y2));

		g.drawLine(x1,y1,x2,y2);

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

	mouseX = e.getPosition().getX();
	mouseY = e.getPosition().getY();

    //[/UserCode_mouseMove]
}

void SynthEditor::mouseDown (const MouseEvent& e)
{
    //[UserCode_mouseDown] -- Add your code here...


	if (e.mods.isLeftButtonDown()) {

		isLeftMouseDown = true;

		minrepaintx = e.getPosition().x;
		minrepainty = e.getPosition().y;

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

		for (int i = 0; i < connections.size(); i++) {
			Connection* c = connections.at(i);

			int x1 = c->source->getX() + c->a->x;
			int y1 = c->source->getY() + c->a->y + 5;
			int x2 = c->target->getX() + c->b->x;
			int y2 = c->target->getY() + c->b->y + 5;

			if (PointOnLineSegment(Point<int>(x1, y1), Point<int>(x2, y2), Point<int>(mouseX, mouseY), 5)) {
				c->selected = true;
			}
			else {
				c->selected = false;
			}
		}
	}
	else if (e.mods.isRightButtonDown()) {

		PopupMenu m;

		Module* module = nullptr;

		for (int i = 0; i < modules.size(); i++) {
			if (modules.at(i)->isSelected()) {
				module = modules.at(i);
			}
		}

		if (module != nullptr) {
			m.addItem(1, "Add input");
			m.addItem(2, "Add output");

			const int result = m.show();

			if (result == 0)
			{
				// user dismissed the menu without picking anything
			}
			else if (result == 1) {
				module->addPin(Pin::Direction::IN);

			}
			else if (result == 2) {
				module->addPin(Pin::Direction::OUT);
			}

		}
		else {
			m.addItem(1, "Add module");
			m.addItem(2, "Save");
            m.addItem(3, "Load");

			const int result = m.show();

			if (result == 0)
			{
				// user dismissed the menu without picking anything
			}
			else if (result == 1)
			{

				Module* m3 = new Module("Macro");

				m3->setTopLeftPosition(500, 100);
				m3->setIndex(Time::currentTimeMillis());

				addAndMakeVisible(m3);
				modules.push_back(m3);

			}
			else if (result == 2)
			{
                FileChooser chooser("Select target file...", File::nonexistent, "*");
                
                if (chooser.browseForFileToSave(true)) {
                    
                    ValueTree v = ValueTree("Synth");
                    
                    ValueTree mods = ValueTree("Modules");
                    
                    for (std::vector<Module*>::iterator it = modules.begin(); it != modules.end(); ++it) {
                        ValueTree file = ValueTree("Module");
                        file.setProperty("name",(*it)->getName(), nullptr);
                        file.setProperty("index",String((*it)->getIndex()), nullptr);
                        file.setProperty("x",(*it)->getPosition().getX(),nullptr);
                        file.setProperty("y",(*it)->getPosition().getY(),nullptr);
                        
                        ValueTree pins = ValueTree("Pins");
                        
                        for (std::vector<Pin*>::iterator it2 =  (*it)->pins.begin(); it2 != (*it)->pins.end(); ++it2) {
                            ValueTree pin = ValueTree("Pin");
                            pin.setProperty("type", (*it2)->type, nil);
                            pin.setProperty("direction", (*it2)->direction, nil);
                            pins.addChild(pin,-1,nil);
                        }
                        
                        
                        
                        mods.addChild(file,-1, nullptr);
                        file.addChild(pins,-1, nullptr);
                    }
                    
                    ValueTree cons = ValueTree("Connections");
                    
                    for (std::vector<Connection*>::iterator it = connections.begin(); it != connections.end(); ++it) {
                        ValueTree con = ValueTree("Connection");
                        con.setProperty("source", String((*it)->source->getIndex()), nullptr);
                        con.setProperty("target", String((*it)->target->getIndex()), nullptr);
                        
                        cons.addChild(con,-1, nullptr);
                    }
                    
                    v.addChild(mods, -1, nullptr);
                    v.addChild(cons, -1, nullptr);

                    File file = chooser.getResult();
                    
                    XmlElement* xml = v.createXml();
                    xml->writeToFile(file, "");
                    delete xml;

                }
			}
            else if (result == 3) {
                FileChooser chooser("Select file to open", File::nonexistent, "*");
                
                if (chooser.browseForFileToOpen()) {
                    
                    File file = chooser.getResult();
                
                    ScopedPointer<XmlElement> xml = XmlDocument(file).getDocumentElement();
                    ValueTree v = ValueTree::fromXml(*xml.get());
                    xml = nullptr;
                    
                    ValueTree mods = v.getChildWithName("Modules");
                    
                    for (int i = 0; i < mods.getNumChildren();i++) {
                        ValueTree mod = mods.getChild(i);
                        
                        Module* m = new Module(mod.getProperty("name"));
                        m->setIndex(mod.getProperty("index").toString().getLargeIntValue());
                        m->setTopLeftPosition(mod.getProperty("x").toString().getIntValue(), mod.getProperty("y").toString().getIntValue());
                        
                        ValueTree pins = mod.getChildWithName("Pins");
                        
                        for (int j = 0; j < pins.getNumChildren();j++) {
                            ValueTree pin = pins.getChild(j);
                            Pin* p = new Pin();
                            
                            int type = pin.getProperty("type").toString().getIntValue();
                            int direction = pin.getProperty("direction").toString().getIntValue();

                            Pin::Direction dir = static_cast<Pin::Direction>(direction);
                            p->direction = dir;
                            Pin::Type t = static_cast<Pin::Type>(type);
                            p->type = t;
                            m->addPin(p->direction);
                        }
                        
                        modules.push_back(m);
                        addAndMakeVisible(m);
                        
                    }
                    
                    ValueTree cons = v.getChildWithName("Connection");
                    
                    for (int i = 0; i < cons.getNumChildren();i++) {
                        ValueTree com = cons.getChild(i);
                        
                        Connection* c = new Connection();
                
                    }
                
                    repaint();
                }
            }
		}

	}

    repaint();

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

void SynthEditor::mouseDoubleClick (const MouseEvent& e)
{
    //[UserCode_mouseDoubleClick] -- Add your code here...

    if (isAltDown) {
        for (int i = 0; i < modules.size(); i++) {

            Module* m = modules.at(i);

            if (m->isSelected()) {
                m->setEditing(true);
                break;
            }
        }
    }
    else {
        for (int i = 0; i < modules.size(); i++) {

            Module* m = modules.at(i);

            if (m->isSelected()) {
                SynthEditor* editor = new SynthEditor();
                editor->setModules(m->getModules());
                editor->setConnections(m->getConnections());
                tab->addTab(m->getName(), juce::Colours::grey,editor, true);
            }
        }

    }


    //[/UserCode_mouseDoubleClick]
}

bool SynthEditor::keyPressed (const KeyPress& key)
{
    //[UserCode_keyPressed] -- Add your code here...

	if (key.getKeyCode() == KeyPress::deleteKey) {
        deleteSelected();
		repaint();
	}

    return true;  // Return true if your handler uses this key event, or false to allow it to be passed-on.
    //[/UserCode_keyPressed]
}

bool SynthEditor::keyStateChanged (bool isKeyDown)
{
    //[UserCode_keyStateChanged] -- Add your code here...
    return false;  // Return true if your handler uses this key event, or false to allow it to be passed-on.
    //[/UserCode_keyStateChanged]
}

void SynthEditor::modifierKeysChanged (const ModifierKeys& modifiers)
{
    //[UserCode_modifierKeysChanged] -- Add your code here...
    isAltDown = modifiers.isAltDown();
    //[/UserCode_modifierKeysChanged]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...

void SynthEditor::setTab(juce::TabbedComponent *t) {
    this->tab = t;
}

void SynthEditor::deleteSelected() {
    for (int i = 0; i < connections.size(); i++) {
        Connection* c = connections.at(i);
        if (c->selected) {
            delete c;
            connections.erase(connections.begin() + i);
        }
    }

    for (int i = 0; i < modules.size(); i++) {

        Module* m = modules.at(i);

        for (int i = 0; i < connections.size(); i++) {
            Connection* c = connections.at(i);
            if (c->source == m || c->target == m) {
                delete c;
                connections.erase(connections.begin() + i);
            }
        }

        if (m->isSelected()) {
            delete m;
            modules.erase(modules.begin() + i);
        }
    }
}


void SynthEditor::checkForPinSelection(const MouseEvent& e, Module* m) {

	for (int j = 0; j < m->pins.size(); j++) {

		if (m->isMouseOverPin(j, e.getPosition())) {
			m->pins.at(j)->selected = true;
		}
		else {
			m->pins.at(j)->selected = false;
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

		if (source->pins.at(j)->selected) {
			a = source->pins.at(j);
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

			if (m->pins.at(j)->selected) {
				b = m->pins.at(j);
				target = m;
				break;
			}

		}

	}

	if (source != nullptr && target != nullptr && a != nullptr && b != nullptr) {
		Logger::writeToLog("Adding new connection");

		Connection* c = new Connection(source, a, target, b);
		connections.push_back(c);
		repaint();
	}

}

bool SynthEditor::PointOnLineSegment(Point<int> pt1, Point<int> pt2, Point<int> pt, double epsilon = 0.001)
{
	if (pt.x - std::max(pt1.x, pt2.x) > epsilon ||
		std::min(pt1.x, pt2.x) - pt.x > epsilon ||
		pt.y - std::max(pt1.y, pt2.y) > epsilon ||
		std::min(pt1.y, pt2.y) - pt.y > epsilon)
		return false;

	if (abs(pt2.x - pt1.x) < epsilon)
		return abs(pt1.x - pt.x) < epsilon ||abs(pt2.x - pt.x) < epsilon;
	if (abs(pt2.y - pt1.y) < epsilon)
		return abs(pt1.y - pt.y) < epsilon || abs(pt2.y - pt.y) < epsilon;

	double x = pt1.x + (pt.y - pt1.y) * (pt2.x - pt1.x) / (pt2.y - pt1.y);
	double y = pt1.y + (pt.x - pt1.x) * (pt2.y - pt1.y) / (pt2.x - pt1.x);

	return abs(pt.x - x) < epsilon ||abs(pt.y - y) < epsilon;
}

void SynthEditor::setConnections(std::vector<Connection *> connections) {
    this->connections = connections;
}

void SynthEditor::setModules(std::vector<Module *> modules) {
    this->modules = modules;
}

std::vector<Connection*> SynthEditor::getConnections() {
    return connections;
}

std::vector<Module*> SynthEditor::getModules() {
    return modules;
}


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
    <METHOD name="keyStateChanged (bool isKeyDown)"/>
    <METHOD name="mouseDoubleClick (const MouseEvent&amp; e)"/>
  </METHODS>
  <BACKGROUND backgroundColour="ff323e44"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]
