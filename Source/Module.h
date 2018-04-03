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

#pragma once

//[Headers]     -- You can add your own extra header files here --
#include "../JuceLibraryCode/JuceHeader.h"
#include "MultiComponentDragger.h"
#include <vector>
#include "Pin.h"
// #include "Connection.h"

class Connection;

//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Projucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class Module  : public Component,
                public TextEditor::Listener
{
public:
    //==============================================================================
    Module ();
    ~Module();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    void addPin(Pin::Direction direction);

	Module(String name);

    Pin* getSelectedPin();

    std::vector<Pin*> pins;

	bool isMouseOverPin(int pin, Point<int> pos);

	Point<int> getPinPosition(int i);

    void setSelected(bool selected);
    bool isSelected();

	long getIndex();
	void setIndex(long index);
	void setEditing(bool editing);

    std::vector<Connection*> getConnections();
    std::vector<Module*> getModules();

    void setConnections(std::vector<Connection*> connections);

    void textEditorReturnKeyPressed(juce::TextEditor &) override;
    void setModules(std::vector<Module*> modules);

    //[/UserMethods]

    void paint (Graphics& g) override;
    void resized() override;
    void mouseDoubleClick (const MouseEvent& e) override;



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    MultiComponentDragger* dragger;

    bool selected = false;
	long index;

    std::vector<Module*> modules;
    std::vector<Connection*> connections;

    //[/UserVariables]

    //==============================================================================
    ScopedPointer<TextEditor> nameEditor;
    ScopedPointer<Label> nameLabel;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Module)
};

//[EndFile] You can add extra defines here...
//[/EndFile]
