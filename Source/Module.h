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
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Projucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class Module  : public Component
{
public:
    //==============================================================================
    Module ();
    ~Module();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    void addInput();
    void addOutput();

    Pin* getSelectedPin();

    std::vector<Pin> inputs;
    std::vector<Pin> outputs;

	bool isMouseOverInput(int pin, Point<int> pos);
	bool isMouseOverOutput(int pin, Point<int> pos);
    
    //[/UserMethods]

    void paint (Graphics& g) override;
    void resized() override;

    void setSelected(bool selected);
    bool isSelected();

private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    MultiComponentDragger* dragger;

    bool selected = false;

    //[/UserVariables]

    //==============================================================================


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Module)
};

//[EndFile] You can add extra defines here...
//[/EndFile]
