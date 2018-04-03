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
#include "Module.h"
#include "Connection.h"
#include <vector>
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Projucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class SynthEditor  : public Component
{
public:
    //==============================================================================
    SynthEditor ();
    ~SynthEditor();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

	void checkForPinSelection(const MouseEvent& e, Module* m);
	void addConnection(const MouseEvent& e, Module* m);
	bool PointOnLineSegment(Point<int> pt1, Point<int> pt2, Point<int> pt, double epsilon);

    //[/UserMethods]

    void paint (Graphics& g) override;
    void resized() override;
    void mouseMove (const MouseEvent& e) override;
    void mouseDown (const MouseEvent& e) override;
    void mouseDrag (const MouseEvent& e) override;
    void mouseUp (const MouseEvent& e) override;
    bool keyPressed (const KeyPress& key) override;
    void modifierKeysChanged (const ModifierKeys& modifiers) override;



private:
    //[UserVariables]   -- You can add your own custom variables in this section.

    bool isAltDown = false;
    bool isLeftMouseDown = false;
	bool isMovingModule = true;

    std::vector<Module*> modules;

	int mouseX = 0;
	int mouseY = 0;

    int lineStartX = 0;
    int lineStartY = 0;
    int lineStopX = 0;
    int lineStopY = 0;

	int dragStartX = 0;
	int dragStartY = 0;

	int maxrepaintw = 0;
	int maxrepainth = 0;
	int minrepaintx = 0;
	int minrepainty = 0;

	std::vector<Connection*> connections;

    //[/UserVariables]

    //==============================================================================


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthEditor)
};

//[EndFile] You can add extra defines here...
//[/EndFile]
