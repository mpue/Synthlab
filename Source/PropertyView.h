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
#include "Constant.h"
#include "Knob.h"
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
            if (view->currentEditor->getSelectedModule() != NULL) {
                view->currentEditor->getSelectedModule()->setName(value.toString());
            }
        }
        PropertyView* view;
        Value value;
    };
    
    struct ValueListener : Value::Listener
    {
        ValueListener (Value& v, PropertyView* view) : value (v), view(view){ value.addListener (this); }
        ~ValueListener()  {}  // no need to remove the listener
        
        void valueChanged (Value& value) override {
            
            if (view->currentEditor->getSelectedModule() != nullptr) {
                
                Constant* c = nullptr;
            
                
                if ((c = dynamic_cast<Constant*>(view->currentEditor->getSelectedModule())) != NULL) {
                    c->setValue(value.toString().getFloatValue());
                }
                            
                
            }
            
            
        }
        PropertyView* view;
        Value value;
    };
    
    struct MaxValueListener : Value::Listener
    {
        MaxValueListener (Value& v, PropertyView* view) : value (v), view(view){ value.addListener (this); }
        ~MaxValueListener()  {}  // no need to remove the listener
        
        void valueChanged (Value& value) override {
            
            if (view->currentEditor->getSelectedModule() != nullptr) {
                
                Knob* k = nullptr;
                
                
                if ((k = dynamic_cast<Knob*>(view->currentEditor->getSelectedModule())) != NULL) {
                    k->setMaximum(value.toString().getFloatValue());
                }
                
                
            }
            
            
        }
        PropertyView* view;
        Value value;
    };
    
    struct MinValueListener : Value::Listener
    {
        MinValueListener (Value& v, PropertyView* view) : value (v), view(view){ value.addListener (this); }
        ~MinValueListener()  {}  // no need to remove the listener
        
        void valueChanged (Value& value) override {
            
            if (view->currentEditor->getSelectedModule() != nullptr) {
                
                Knob* k = nullptr;
                
                
                if ((k = dynamic_cast<Knob*>(view->currentEditor->getSelectedModule())) != NULL) {
                    k->setMinimum(value.toString().getFloatValue());
                }
                
                
            }
            
            
        }
        PropertyView* view;
        Value value;
    };
    
    struct StepsizeValueListener : Value::Listener
    {
        StepsizeValueListener (Value& v, PropertyView* view) : value (v), view(view){ value.addListener (this); }
        ~StepsizeValueListener()  {}  // no need to remove the listener
        
        void valueChanged (Value& value) override {
            
            if (view->currentEditor->getSelectedModule() != nullptr) {
                
                Knob* k = nullptr;
                
                
                if ((k = dynamic_cast<Knob*>(view->currentEditor->getSelectedModule())) != NULL) {
                    k->setStepSize(value.toString().getFloatValue());
                }
                
                
            }
            
            
        }
        PropertyView* view;
        Value value;
    };
    
    String name;

    SynthEditor* currentEditor;
    PropertyPanel* propertyPanel = nullptr;
    
    Value* nameValue;
    Value* value;
    Value* numInputsValue;
    Value* numOutputsValue;
    Value* minValue;
    Value* maxValue;
    Value* stepsizeValue;
    
    NameListener* nameListener;
    ValueListener* valueListener;
    MinValueListener* minValueListener;
    MaxValueListener* maxValueListener;
    StepsizeValueListener* stepsizeValueListener;
    
    juce::Array<PropertyComponent*> properties;
    
    PropertyComponent* nameProp;
    PropertyComponent* inputsProp;
    PropertyComponent* valueProp;
    PropertyComponent* minValueProp;
    PropertyComponent* maxValueProp;
    PropertyComponent* stepsizeValueProp;
    
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<MainTabbedComponent> tabbedComponent;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PropertyView)
};

//[EndFile] You can add extra defines here...
//[/EndFile]
