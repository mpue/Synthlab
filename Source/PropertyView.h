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
#include "MainTabbedComponent.h"
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Projucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class PropertyView  : public Component,
                      public ChangeListener
{
public:
    //==============================================================================
    PropertyView ();
    ~PropertyView();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

    void changeListenerCallback (ChangeBroadcaster* source) override;

    //[/UserMethods]

    void paint (Graphics& g) override;
    void resized() override;



private:
    //[UserVariables]   -- You can add your own custom variables in this section.

    struct NameListener : Value::Listener
    {
        NameListener (Value& v, PropertyView* view) : value (v), view(view){ value.addListener (this); }
        ~NameListener()  {}  // no need to remove the listener

        void valueChanged (Value& value) override {
            view->currentEditor->getSelectedModule()->setName(value.toString());
        }
        PropertyView* view;
        Value value;
    };
    
    String name;

    SynthEditor* currentEditor;
    PropertyPanel* propertyPanel;
    Value* nameValue;
    Value* numInputsValue;
    Value* numOutputsValue;
    NameListener* nameListener;
    juce::Array<PropertyComponent*> properties;
    PropertyComponent* nameProp;
    PropertyComponent* inputsProp;

    //[/UserVariables]

    //==============================================================================
    ScopedPointer<MainTabbedComponent> tabbedComponent;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PropertyView)
};

//[EndFile] You can add extra defines here...
//[/EndFile]
